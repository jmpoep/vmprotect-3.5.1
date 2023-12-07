namespace forms_cil
{
	partial class Form1
	{
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.IContainer components = null;

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		/// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
		protected override void Dispose(bool disposing)
		{
			if (disposing && (components != null))
			{
				components.Dispose();
			}
			base.Dispose(disposing);
		}

		#region Windows Form Designer generated code

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.label1 = new System.Windows.Forms.Label();
			this.txtPassword = new System.Windows.Forms.TextBox();
			this.btnCheckPassword = new System.Windows.Forms.Button();
			this.SuspendLayout();
			// 
			// label1
			// 
			this.label1.AutoSize = true;
			this.label1.Location = new System.Drawing.Point(13, 13);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(56, 13);
			this.label1.TabIndex = 0;
			this.label1.Text = "Password:";
			// 
			// txtPassword
			// 
			this.txtPassword.Location = new System.Drawing.Point(75, 10);
			this.txtPassword.Name = "txtPassword";
			this.txtPassword.Size = new System.Drawing.Size(192, 20);
			this.txtPassword.TabIndex = 1;
			// 
			// btnCheckPassword
			// 
			this.btnCheckPassword.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.btnCheckPassword.Location = new System.Drawing.Point(273, 8);
			this.btnCheckPassword.Name = "btnCheckPassword";
			this.btnCheckPassword.Size = new System.Drawing.Size(98, 23);
			this.btnCheckPassword.TabIndex = 2;
			this.btnCheckPassword.Text = "Check password";
			this.btnCheckPassword.UseVisualStyleBackColor = true;
			this.btnCheckPassword.Click += new System.EventHandler(this.btnCheckPassword_Click);
			// 
			// Form1
			// 
			this.AcceptButton = this.btnCheckPassword;
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(384, 40);
			this.Controls.Add(this.btnCheckPassword);
			this.Controls.Add(this.txtPassword);
			this.Controls.Add(this.label1);
			this.Name = "Form1";
			this.Text = "VMProtect test [FormsCIL]";
			this.ResumeLayout(false);
			this.PerformLayout();

		}

		#endregion

		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox txtPassword;
		private System.Windows.Forms.Button btnCheckPassword;
	}
}

