namespace UnitTestProject.RefVm
{
    // Token: 0x02000028 RID: 40
    public sealed class VmMethodHeader // \u0006
    {
        // Token: 0x06000120 RID: 288 RVA: 0x00005F08 File Offset: 0x00004108
        // Token: 0x06000121 RID: 289 RVA: 0x00005F10 File Offset: 0x00004110
        // Token: 0x04000047 RID: 71
        public LocalVarType[] LocalVarTypes /* \u0003 */ { get; set; }

        // Token: 0x06000122 RID: 290 RVA: 0x00005F1C File Offset: 0x0000411C
        // Token: 0x06000123 RID: 291 RVA: 0x00005F24 File Offset: 0x00004124
        // Token: 0x0400004B RID: 75
        public ArgTypeToOutput[] ArgsTypeToOutput /* \u000E */ { get; set; }

        // Token: 0x06000124 RID: 292 RVA: 0x00005F30 File Offset: 0x00004130
        // Token: 0x06000125 RID: 293 RVA: 0x00005F38 File Offset: 0x00004138
        // Token: 0x04000048 RID: 72
        public string Name /* \u0002 */ { get; set; }

        // Token: 0x06000126 RID: 294 RVA: 0x00005F44 File Offset: 0x00004144
        // Token: 0x06000127 RID: 295 RVA: 0x00005F4C File Offset: 0x0000414C
        // Token: 0x04000046 RID: 70
        public int ClassId /* \u0002 */ { get; set; }

        // Token: 0x06000128 RID: 296 RVA: 0x00005F58 File Offset: 0x00004158
        // Token: 0x06000129 RID: 297 RVA: 0x00005F60 File Offset: 0x00004160
        // Token: 0x04000049 RID: 73
        public int ReturnTypeId /* \u0008 */ { get; set; }

        // Token: 0x0600012A RID: 298 RVA: 0x00005F6C File Offset: 0x0000416C
        // Token: 0x0600012B RID: 299 RVA: 0x00005F74 File Offset: 0x00004174
        // Token: 0x0400004A RID: 74
        public byte Flags  /* \u0006 */ { private get; set; }

	    // Token: 0x0600012C RID: 300 RVA: 0x00005F80 File Offset: 0x00004180
	    public bool IsStatic() // \u0002
	    {
		    return (Flags & 2) > 0;
	    }
    }
}
