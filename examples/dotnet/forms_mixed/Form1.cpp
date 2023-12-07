using namespace System;
using namespace System::Drawing;
using namespace System::Windows;
using namespace System::Windows::Forms;
using namespace System::Runtime::InteropServices;


#pragma unmanaged
#include "stdlib.h"
bool CheckPassword(const char *pwd)
{
	__int64 val = _atoi64(pwd);
	return (val% 0x11) == 13;
}
#pragma managed

namespace forms_mixed
{
	public ref class Form1 : public Form
	{
	public: 
		Form1()
		{
			label1 = gcnew Label();
			txtPassword = gcnew TextBox();
			btnCheckPassword = gcnew Button();
			Form::SuspendLayout();
			label1->AutoSize = true;
			label1->Location = Point(13, 13);
			label1->Name = "label1";
			label1->Size = System::Drawing::Size(0x38, 13);
			label1->TabIndex = 0;
			label1->Text = "Password:";
			txtPassword->Location = Point(0x4b, 10);
			txtPassword->Name = "txtPassword";
			txtPassword->Size = System::Drawing::Size(0xc0, 20);
			txtPassword->TabIndex = 1;
			btnCheckPassword->DialogResult = Forms::DialogResult::OK;
			btnCheckPassword->Location = Point(0x111, 8);
			btnCheckPassword->Name = "btnCheckPassword";
			btnCheckPassword->Size = System::Drawing::Size(0x62, 0x17);
			btnCheckPassword->TabIndex = 2;
			btnCheckPassword->Text = "Check password";
			btnCheckPassword->UseVisualStyleBackColor = true;
			btnCheckPassword->Click += gcnew EventHandler(this, &Form1::btnCheckPassword_Click);
			Form::AcceptButton = btnCheckPassword;
			Form::AutoScaleDimensions = System::Drawing::Size(6, 13);
			Form::AutoScaleMode = Forms::AutoScaleMode::Font;
			Form::ClientSize = System::Drawing::Size(0x180, 40);
			Form::Controls->Add(btnCheckPassword);
			Form::Controls->Add(txtPassword);
			Form::Controls->Add(label1);
			Form::Name = "Form1";
			Text = "VMProtect test [FormsMixed]";
			Form::ResumeLayout(false);
			Form::PerformLayout();
		}

	private:
		[VMProtect::Begin]
		void btnCheckPassword_Click(Object^ sender, EventArgs^ e)
		{
			IntPtr pwd = Marshal::StringToCoTaskMemAnsi(txtPassword->Text);
			try
			{
				if (CheckPassword((const char *)((void *)pwd)))
				{
					MessageBox::Show(VMProtect::SDK::DecryptString("Correct password"), VMProtect::SDK::DecryptString("Password check"), MessageBoxButtons::OK, MessageBoxIcon::Asterisk);
				}
				else
				{
					MessageBox::Show(VMProtect::SDK::DecryptString("Incorrect password"), VMProtect::SDK::DecryptString("Password check"), MessageBoxButtons::OK, MessageBoxIcon::Hand);
					txtPassword->Focus();
				}
			} catch(...)
			{
				Marshal::FreeCoTaskMem(pwd);
				throw;
			}
		}

		Button^ btnCheckPassword;
		System::ComponentModel::IContainer^ components;
		Label^ label1;
		TextBox^ txtPassword;
	};
}

int __stdcall WinMain (void *hInstance, void *hPrevInstance, void *lpCmdLine, int nShowCmd)
{
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false);
	Application::Run(gcnew forms_mixed::Form1());
	return 0;
}
