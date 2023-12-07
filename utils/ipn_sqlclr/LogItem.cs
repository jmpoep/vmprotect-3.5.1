using System.Xml;

namespace ipn_sqlclr
{
	public class LogItem
	{
		public int MsgId { get; set; }
		public XmlDocument[] Xml { get; set; }
		public string[] P { get; set; }
	}
}
