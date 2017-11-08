// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT

#include <Launcher/Widgets/MainWindow.hpp>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <iostream>

MainWindow::MainWindow()
{
	resize(1024, 768);
	
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->addSpacing(200);

	m_newsWidget = new QTextEdit;
	m_newsWidget->setReadOnly(true);
	m_newsWidget->setHtml("<b>Downloading news feed...</b>");
	m_newsWidget->setMinimumSize(640, 480);
	//m_newsWidget->setContentsMargins(QMargins(50, 50, 50, 50));
	//m_newsWidget->setSizePolicy(QSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum));

	mainLayout->addWidget(m_newsWidget, 0, Qt::AlignCenter);

	m_statusLabel = new QLabel;
	mainLayout->addWidget(m_statusLabel, 0, Qt::AlignHCenter);

	QHBoxLayout* bottomLayout = new QHBoxLayout;

	m_progressBar = new QProgressBar;
	bottomLayout->addWidget(m_progressBar);

	QPushButton* startButton = new QPushButton("Play");
	bottomLayout->addWidget(startButton);

	mainLayout->addLayout(bottomLayout);

	setLayout(mainLayout);

	m_statusLabel->setText("<b>Downloading news feed...</b>");

	DownloadNewsFeed();
	StartUpdateProcess();
}

MainWindow::~MainWindow()
{
}

void MainWindow::DownloadNewsFeed()
{
	QNetworkRequest request(QUrl("https://utopia.digitalpulsesoftware.net/news"));
	QNetworkReply* download = m_network.get(request);
	connect(download, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, download] (QNetworkReply::NetworkError code) { m_newsWidget->setText("Failed to query news: " + download->errorString()); });
	connect(download, &QNetworkReply::finished, [this, download] () { m_newsWidget->setText(download->readAll()); download->deleteLater(); });
	//connect(download, &QNetworkReply::readyRead, [this, download] () { m_newsWidget->append(download->readAll()); });
}

void MainWindow::StartUpdateProcess()
{
	QByteArray* buffer = new QByteArray;

	QNetworkRequest request(QUrl("https://utopia.digitalpulsesoftware.net/manifest"));
	QNetworkReply* download = m_network.get(request);
	connect(download, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, download, buffer] (QNetworkReply::NetworkError /*code*/) { buffer->clear(); OnManifestDownloadError(download); });
	connect(download, &QNetworkReply::downloadProgress, [this, download] (qint64 bytesReceived, qint64 bytesTotal) { m_progressBar->setValue(100 * bytesTotal / std::max(bytesReceived, qint64(1))); });
	connect(download, &QNetworkReply::finished, [this, download, buffer] () { download->deleteLater(); OnManifestDownloaded(*buffer); delete buffer; });
	connect(download, &QNetworkReply::readyRead, [this, download, buffer] () { buffer->append(download->readAll()); });
}

void MainWindow::OnFileDownloadError(const QString& filePath, QNetworkReply* download)
{
	m_statusLabel->setText("<b> Failed to download " + filePath + ": " + ((download) ? download->errorString() : "Failed to open file") + "</b>");
}

void MainWindow::OnManifestDownloaded(const QByteArray& buffer)
{
	std::cout << buffer.toStdString() << std::endl;
	QJsonParseError parseError;
	QJsonDocument manifest = QJsonDocument::fromJson(buffer, &parseError);
	if (manifest.isNull())
	{
		m_statusLabel->setText("<b>Manifest parsing error: " + parseError.errorString() + "</b>");
		return;
	}

	QJsonObject object = manifest.object();
	for (const QJsonValue& entry : object["Directories"].toArray())
		QDir().mkpath(entry.toString());

	m_downloadList.clear();
	for (const QJsonValue& entry : object["Files"].toArray())
	{
		QJsonObject fileData = entry.toObject();
		QString path = fileData["path"].toString();
		int size = fileData["size"].toInt();
		QString hash = fileData["hash"].toString();

		if (QFile::exists(path))
		{
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly))
			{
				std::cerr << "Failed to open: " << path.toStdString() << std::endl;
				continue;
			}

			qint64 fileSize = file.size();

			if (fileSize == size)
			{
				QCryptographicHash fileHash(QCryptographicHash::Sha1);
				if (fileHash.addData(&file))
				{
					if (hash == fileHash.result().toHex())
						continue;
				}
			}
		}

		// There was a problem with this file, re-download it
		std::cout << "Invalid file:" << path.toStdString() << std::endl;

		m_downloadList.emplace_back(std::move(path));
	}

	m_downloadCounter = 0;
	ProcessDownloadList();
}

void MainWindow::OnManifestDownloadError(QNetworkReply* download)
{
	m_statusLabel->setText("<b>Failed to download manifest</b>");
}

void MainWindow::ProcessDownloadList()
{
	if (m_downloadCounter >= m_downloadList.size())
		return;

	QString filePath = m_downloadList[m_downloadCounter++];

	m_progressBar->setValue(0);
	m_statusLabel->setText("<b> Downloading " + filePath + " (" + QString::number(m_downloadCounter) + '/' + QString::number(m_downloadList.size()) +")</b>");

	QFile* fileOutput = new QFile(filePath);
	if (!fileOutput->open(QIODevice::WriteOnly))
	{
		OnFileDownloadError(filePath, nullptr);
		ProcessDownloadList();
		return;
	}

	QNetworkRequest request { QUrl("https://utopia.digitalpulsesoftware.net/" + filePath) };
	QNetworkReply* download = m_network.get(request);
	connect(download, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, download, filePath] (QNetworkReply::NetworkError /*code*/) { OnFileDownloadError(filePath, download); });
	connect(download, &QNetworkReply::downloadProgress, [this, download] (qint64 bytesReceived, qint64 bytesTotal) { m_progressBar->setValue(100 * bytesTotal / std::max(bytesReceived, qint64(1))); });
	connect(download, &QNetworkReply::finished, [this, download, fileOutput] () { ProcessDownloadList(); download->deleteLater(); delete fileOutput; });
	connect(download, &QNetworkReply::readyRead, [this, download, fileOutput] () { fileOutput->write(download->readAll()); });
}


//#include <Application/Widgets/MainWindow.moc>
