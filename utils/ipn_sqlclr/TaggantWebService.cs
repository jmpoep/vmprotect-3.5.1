using System;
using System.Collections.Generic;
using System.IO;
using System.Xml;
using certificateManagementService;
using Org.BouncyCastle.Asn1.X509;
using Org.BouncyCastle.Crypto;
using Org.BouncyCastle.OpenSsl;
using Org.BouncyCastle.Pkcs;
using Org.BouncyCastle.Security;
using Org.BouncyCastle.X509;
using policyService;
using veriSignCertIssuingService;
using ItemChoiceType = certificateManagementService.ItemChoiceType;

namespace ipn_sqlclr
{
	public static class TaggantWebService
	{
		public static void CertRevoke(TaggantConfig tc, string id, List<LogItem> log)
		{
			var es = new certificateManagementService.certificateManagementService(tc.ClientCertificate, tc["ManagementUrl"]);
			try
			{
				var updateCertificateStatusRequest = new UpdateCertificateStatusRequestType
				{
					clientTransactionID = "ipn_sqlclr " + new SecureRandom().Next(),
					operationType = OperationTypeEnum.Revoke,
					revocationReasonSpecified = false,
					ItemElementName = ItemChoiceType.seatId,
					Item = id,
					//certificateIssuer = "?",
					//challenge = "?",
					//comment = "?",
					version = tc["ManagementVersion"]
				};
				/*var updateResponse =*/ es.updateCertificateStatus(updateCertificateStatusRequest);
			}
			finally
			{
				LogXml("updateCertificateStatus", es, log);
			}
		}
		public static string CertRequestNew(TaggantConfig tc, string id, string mail, string privateKey, List<LogItem> log)
		{
			var csr = CreateCsr(tc, privateKey);
			log.Add(new LogItem {MsgId = 16, P = new[] {csr}});

			var es = new veriSignCertIssuingService.veriSignCertIssuingService(tc.ClientCertificate, tc["EnrollmentUrl"]);
			try
			{
				var requestSecurityTokenType = new RequestSecurityTokenType
				{
					Item = new RequestVSSecurityTokenEnrollmentType
					{
						clientTransactionID = "ipn_sqlclr " + new SecureRandom().Next(),
						certificateProfileID = tc["CertificateProfileOid"],
						requestType = RequestTypeEnum.httpdocsoasisopenorgwssxwstrust200512Issue,
						version = tc["EnrollVersion"],
						tokenType = TokenType.httpdocsoasisopenorgwss200401oasis200401wssx509tokenprofile10PKCS7,
						binarySecurityToken = new[]
						{
							new BinarySecurityTokenType
							{
								ValueType = "http://schemas.verisign.com/pkiservices/2009/07/PKCS10",
								EncodingType = "http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd#base64binary",
								Value = csr
							}
						},
						nameValuePair = new[]
						{
							new NameValueType {name = "seat_id", value = mail},
							new NameValueType {name = "common_name", value = string.Format("VMProtect Client {0}", id)},
							new NameValueType {name = "mail_lastName", value = "Client"},
							new NameValueType {name = "mail_firstName", value = string.Format("{0} VMProtect", id)},
							new NameValueType {name = "emailAddress", value = mail},
							new NameValueType {name = "mail_email", value = mail},
							new NameValueType {name = "country", value = "ru"}
						}
					}
				};
				var enrollmentResponse = es.RequestSecurityToken(requestSecurityTokenType);
				var certs = ((AttributedString)(enrollmentResponse.Item.requestedVSSecurityToken.Items[0])).Value;
				var certPkcs7 = Convert.FromBase64String(certs);
				var parser = new X509CertificateParser();
				var cert = parser.ReadCertificate(certPkcs7);
				using (var pw = new StringWriter())
				{
					new PemWriter(pw).WriteObject(cert);
					pw.Flush();
					return pw.ToString();
				}
			}
			finally
			{
				LogXml("RequestSecurityToken", es, log);
			}
		}

		private static string CreateCsr(TaggantConfig tc, string privateKey)
		{
			AsymmetricCipherKeyPair pair;
			using (var reader = new StringReader(privateKey))
				pair = (AsymmetricCipherKeyPair) new PemReader(reader).ReadObject();

			var csr = new Pkcs10CertificationRequest(tc["CsrAlgorithm"], new X509Name(tc["CsrSubject"]), pair.Public, null, pair.Private);
			using (var pw = new StringWriter())
			{
				new PemWriter(pw).WriteObject(csr);
				pw.Flush();
				return pw.ToString();
			}
		}

		public static IEnumerable<OID> GetPolicies(TaggantConfig tc, List<LogItem> log)
		{
			var ps = new policyService.policyService(tc.ClientCertificate, tc["PolicyUrl"]);
			try
			{
				var rp = ps.requestPolicies(new getPolicies {version = tc["PolicyVersion"]});
				return rp.oIDs;
			}
			finally
			{
				LogXml("requestPolicies", ps, log);
			}
		}

		private static void LogXml(string src, XmlReaderSpyService ss, ICollection<LogItem> log)
		{
			var req = new XmlDocument();
			var resp = new XmlDocument();
			var reqs = ss.GetRequestXml();
			var resps = ss.GetResponseXml();
			try
			{
				req.LoadXml(reqs);
			}
			catch (Exception)
			{
				req = null;
			}
			try
			{
				resp.LoadXml(resps);
			}
			catch (Exception)
			{
				resp = null;
			}
			if (req != null && string.IsNullOrWhiteSpace(req.InnerXml))
				req = null;
			if (resp != null && string.IsNullOrWhiteSpace(resp.InnerXml))
				resp = null;
			if (!string.IsNullOrWhiteSpace(reqs) || !string.IsNullOrWhiteSpace(resps))
				log.Add(new LogItem {MsgId = 17, P = new[] {src, reqs, resps}, Xml = new[] {req, resp}});
		}
	}
}
