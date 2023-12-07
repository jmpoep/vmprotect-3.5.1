#ifdef ULTIMATE
#include "../core/objects.h"
#include "../core/core.h"
#include "../core/lang.h"
#include "export_key_pair_dialog.h"
#include "widgets.h"
#include "moc/moc_export_key_pair_dialog.cc"
#include "help_browser.h"

/**
 * ExportKeyPairDialog
 */

ExportKeyPairDialog::ExportKeyPairDialog(QWidget *parent, LicensingManager *manager)
	: QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint)
{
	bits_ = manager->bits();
	private_exp_ = manager->private_exp();
	public_exp_ = manager->public_exp();
	modulus_ = manager->modulus();
	uint64_t product_code = manager->product_code();
	product_code_.insert(product_code_.end(), reinterpret_cast<uint8_t *>(&product_code), reinterpret_cast<uint8_t *>(&product_code) + sizeof(product_code));

	setWindowTitle(QString::fromUtf8(language[lsExportKeyPair].c_str()));

	QLabel *labelFormat = new QLabel(this);
	labelFormat->setObjectName("header");
	labelFormat->setText(QString::fromUtf8(language[lsKeyPairExportTarget].c_str()));

	QComboBox *format = new QComboBox(this);
	format->addItems(QStringList() << QString::fromUtf8(language[lsParametersForMSVC].c_str())
					<< QString::fromUtf8(language[lsParametersForDelphi].c_str())
					<< QString::fromUtf8(language[lsParametersForNET].c_str())
					<< QString::fromUtf8(language[lsParametersForPHP].c_str()));
	format->setCurrentIndex(0);
	connect(format, SIGNAL(currentIndexChanged(int)), this, SLOT(formatChanged(int)));

	QLabel *labelResults = new QLabel(this);
	labelResults->setObjectName("header");
	labelResults->setText(QString::fromUtf8(language[lsKeyPairExportResult].c_str()));

	edit_ = new QPlainTextEdit(this);
	QFont font = edit_->font();
	font.setFamily(MONOSPACE_FONT_FAMILY);
	edit_->setFont(font);

	QToolButton *helpButton = new QToolButton(this);
	helpButton->setShortcut(HelpContentsKeySequence());
	helpButton->setIconSize(QSize(20, 20));
	helpButton->setIcon(QIcon(":/images/help_gray.png"));
	helpButton->setToolTip(QString::fromUtf8(language[lsHelp].c_str()));
	connect(helpButton, SIGNAL(clicked(bool)), this, SLOT(helpClicked()));

    QPushButton *closeButton = new PushButton(QString::fromUtf8(language[lsClose].c_str()));
	connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));

	QPushButton *copyButton = new PushButton(QString::fromUtf8(language[lsCopy].c_str()));
	connect(copyButton, SIGNAL(clicked()), this, SLOT(copyClicked()));

    QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->addWidget(helpButton);
	buttonLayout->addStretch();
#ifdef __APPLE__
	buttonLayout->addWidget(closeButton);
	buttonLayout->addWidget(copyButton);
#else
	buttonLayout->addWidget(copyButton);
	buttonLayout->addWidget(closeButton);
#endif
    
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(10, 10, 10, 10);
	layout->setSpacing(10);
	layout->addWidget(labelFormat);
	layout->addWidget(format);
	layout->addWidget(labelResults);
	layout->addWidget(edit_);
    layout->addLayout(buttonLayout);
	setLayout(layout);

	resize(610, 300);

	formatChanged(format->currentIndex());
}

QString VectorToString(const std::vector<uint8_t> &value)
{
	QString res;
	for (size_t i = 0; i < value.size(); i++) {
		if (i > 0)
			res.append(',').append((i & 0xf) ? ' ' : '\n');
		res.append(QString("%1").arg(value[i], 3, 10, QChar(' ')));
	}

	return res;
}

QString _VectorToBase64(const std::vector<uint8_t> &value)
{
	QByteArray res = QByteArray(reinterpret_cast<const char *>(&value[0]), (int)value.size());
	return res.toBase64();
}

QString StringToBase64(const QString &value)
{
	QByteArray res;
	res.append(value);
	return res.toBase64();
}

void ExportKeyPairDialog::formatChanged(int format)
{
	QStringList lines;
	switch (format) {
	case 0:
		lines.append("VMProtectAlgorithms g_Algorithm = ALGORITHM_RSA;");
      	lines.append(QString("size_t g_nBits = %1;").arg(bits_));
      	lines.append(QString("byte g_vPrivate[%1] = {").arg(private_exp_.size()));
      	lines.append(VectorToString(private_exp_) + "};");
      	lines.append("");
      	lines.append(QString("byte g_vModulus[%1] = {").arg(modulus_.size()));
      	lines.append(VectorToString(modulus_) + "};");
      	lines.append("");
      	lines.append(QString("byte g_vProductCode[%1] = {%2};").arg(product_code_.size()).arg(VectorToString(product_code_)));
		break;
	case 1:
		lines.append("g_Algorithm: Longword = ALGORITHM_RSA;");
      	lines.append(QString("g_nBits: Longword = %1;").arg(bits_));
      	lines.append(QString("g_vPrivate: array [0..%1] of Byte = (").arg(private_exp_.size() - 1));
      	lines.append(VectorToString(private_exp_) + ");");
      	lines.append("");
      	lines.append(QString("g_vModulus: array [0..%1] of Byte = (").arg(modulus_.size() - 1));
      	lines.append(VectorToString(modulus_) + ");");
      	lines.append("");
      	lines.append(QString("g_vProductCode: array [0..%1] of Byte = (%2);").arg(product_code_.size() - 1).arg(VectorToString(product_code_)));
		break;
	case 2:
		{
			QString str = StringToBase64(QString("<vmp-lm-product algorithm=\"RSA\" bits=\"%1\" exp=\"%2\" mod=\"%3\" product=\"%4\"/>").arg(bits_).arg(_VectorToBase64(private_exp_)).arg(_VectorToBase64(modulus_)).arg(_VectorToBase64(product_code_)));
			while (!str.isEmpty()) {
				lines.append(str.left(76));
				str.remove(0, 76);
			}
		}
		break;
	case 3:
		lines.append("$exported_algorithm = \"RSA\";");
		lines.append(QString("$exported_bits = %1;").arg(bits_));
		lines.append(QString("$exported_private = \"%1\";").arg(_VectorToBase64(private_exp_)));
		lines.append(QString("$exported_modulus = \"%1\";").arg(_VectorToBase64(modulus_)));
		lines.append(QString("$exported_product_code = \"%1\";").arg(_VectorToBase64(product_code_)));
		break;
	}
	edit_->setPlainText(lines.join("\n"));
}

void ExportKeyPairDialog::copyClicked()
{
	 QClipboard *clipboard = QApplication::clipboard();
	 clipboard->setText(edit_->toPlainText());
}

void ExportKeyPairDialog::helpClicked()
{
	HelpBrowser::showTopic("project::licenses");
}

#endif
