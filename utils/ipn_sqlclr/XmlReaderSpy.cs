using System.Security.Cryptography.X509Certificates;
using System.Text;
using System.Web.Services.Protocols;
using System.Xml;
using System.IO;

namespace ipn_sqlclr
{
	public class XmlReaderSpy : StreamReader
	{
		private readonly StringBuilder _sb = new StringBuilder();
		public XmlReaderSpy(Stream stream, Encoding encoding, bool p, int bufferSize) : base(stream, encoding, p, bufferSize)
		{
		}

		public override int Read(char[] buffer, int index, int count)
		{
			var ret = base.Read(buffer, index, count);
			if(ret > 0)
				_sb.Append(buffer, index, ret);
			return ret;
		}

		public override string ReadToEnd()
		{
			var ret = base.ReadToEnd();
			_sb.Append(ret);
			return ret;
		}

		public override string ReadLine()
		{
			var ret = base.ReadLine();
			_sb.Append(ret);
			return ret;
		}

		public override int ReadBlock(char[] buffer, int index, int count)
		{
			return Read(buffer, index, count);
		}

		public string Xml
		{
			get { return _sb.ToString().Replace("<?xml version='1.0' encoding='UTF-8'?>", ""); }
		}
	}

	public class XmlWriterSpy : XmlWriter
	{
		private readonly XmlWriter _base;
		private readonly XmlTextWriter _xtw;
		private readonly StringWriter _sw;

		/// <summary>
		/// Extracted XML.
		/// </summary>
		public string Xml
		{
			get
			{
				return (_sw != null) ? _sw.ToString() : string.Empty;
			}
		}

		public XmlWriterSpy(XmlWriter parent)
		{
			_base = parent;
			_sw = new StringWriter();
			_xtw = new XmlTextWriter(_sw);
		}

		#region Abstract properties and methods that must be implemented

		public override WriteState WriteState
		{
			get
			{
				return _base.WriteState;
			}
		}

		public override void Close()
		{
			_base.Close();
			_xtw.Close();
			_sw.Close();
		}

		public override void Flush()
		{
			_base.Flush();
			_xtw.Flush();
			_sw.Flush();
		}

		public override string LookupPrefix(string ns)
		{
			return _base.LookupPrefix(ns);
		}

		public override void WriteBase64(byte[] buffer, int index, int count)
		{
			_base.WriteBase64(buffer, index, count);
			_xtw.WriteBase64(buffer, index, count);
		}

		public override void WriteCData(string text)
		{
			_base.WriteCData(text);
			_xtw.WriteCData(text);
		}

		public override void WriteCharEntity(char ch)
		{
			_base.WriteCharEntity(ch);
			_xtw.WriteCharEntity(ch);
		}

		public override void WriteChars(char[] buffer, int index, int count)
		{
			_base.WriteChars(buffer, index, count);
			_xtw.WriteChars(buffer, index, count);
		}

		public override void WriteComment(string text)
		{
			_base.WriteComment(text);
			_xtw.WriteComment(text);
		}

		public override void WriteDocType(string name, string pubid, string sysid, string subset)
		{
			_base.WriteDocType(name, pubid, sysid, subset);
			_xtw.WriteDocType(name, pubid, sysid, subset);
		}

		public override void WriteEndAttribute()
		{
			_base.WriteEndAttribute();
			_xtw.WriteEndAttribute();
		}

		public override void WriteEndDocument()
		{
			_base.WriteEndDocument();
			_xtw.WriteEndDocument();
		}

		public override void WriteEndElement()
		{
			_base.WriteEndElement();
			_xtw.WriteEndElement();
		}

		public override void WriteEntityRef(string name)
		{
			_base.WriteEntityRef(name);
			_xtw.WriteEntityRef(name);
		}

		public override void WriteFullEndElement()
		{
			_base.WriteFullEndElement();
			_xtw.WriteFullEndElement();
		}

		public override void WriteProcessingInstruction(string name, string text)
		{
			_base.WriteProcessingInstruction(name, text);
			_xtw.WriteProcessingInstruction(name, text);
		}

		public override void WriteRaw(string data)
		{
			_base.WriteRaw(data);
			_xtw.WriteRaw(data);
		}

		public override void WriteRaw(char[] buffer, int index, int count)
		{
			_base.WriteRaw(buffer, index, count);
			_xtw.WriteRaw(buffer, index, count);
		}

		public override void WriteStartAttribute(string prefix, string localName, string ns)
		{
			_base.WriteStartAttribute(prefix, localName, ns);
			_xtw.WriteStartAttribute(prefix, localName, ns);
		}

		public override void WriteStartDocument(bool standalone)
		{
			_base.WriteStartDocument(standalone);
			_xtw.WriteStartDocument(standalone);
		}

		public override void WriteStartDocument()
		{
			_base.WriteStartDocument();
			_xtw.WriteStartDocument();
		}

		public override void WriteStartElement(string prefix, string localName, string ns)
		{
			_base.WriteStartElement(prefix, localName, ns);
			_xtw.WriteStartElement(prefix, localName, ns);
		}

		public override void WriteString(string text)
		{
			_base.WriteString(text);
			_xtw.WriteString(text);
		}

		public override void WriteSurrogateCharEntity(char lowChar, char highChar)
		{
			_base.WriteSurrogateCharEntity(lowChar, highChar);
			_xtw.WriteSurrogateCharEntity(lowChar, highChar);

		}

		public override void WriteWhitespace(string ws)
		{
			_base.WriteWhitespace(ws);
			_xtw.WriteWhitespace(ws);
		}

		#endregion
	}

	public class XmlReaderSpyService : SoapHttpClientProtocol
	{
		protected XmlReaderSpyService(X509Certificate clientCert, string url)
		{
			Url = url;
			ClientCertificates.Add(clientCert);
		}

		private XmlReaderSpy _xmlReaderSpy;
		private XmlWriterSpy _xmlWriterSpy;

		public string GetRequestXml()
		{
			if (_xmlWriterSpy != null)
				return _xmlWriterSpy.Xml;
			return string.Empty;
		}
		public string GetResponseXml()
		{
			if (_xmlReaderSpy != null)
			{
				return _xmlReaderSpy.Xml;
			}
			return string.Empty;
		}

		protected override XmlReader GetReaderForMessage(SoapClientMessage message, int bufferSize)
		{
			Encoding encoding = Encoding.UTF8;
			if (bufferSize < 0x200)
			{
				bufferSize = 0x200;
			}
			var reader = new XmlTextReader(_xmlReaderSpy = new XmlReaderSpy(message.Stream, encoding, true, bufferSize))
			{
				DtdProcessing = DtdProcessing.Prohibit,
				Normalization = true,
				XmlResolver = null
			};
			return reader;
		}

		protected override XmlWriter GetWriterForMessage(SoapClientMessage message, int bufferSize)
		{
			_xmlWriterSpy = new XmlWriterSpy(base.GetWriterForMessage(message, bufferSize));
			return _xmlWriterSpy;
		}
	}
}
