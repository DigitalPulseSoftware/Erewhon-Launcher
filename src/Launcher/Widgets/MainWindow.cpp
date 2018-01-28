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

MainWindow::MainWindow() :
m_isUpdating(false)
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

	m_startButton = new QPushButton("Play");
	connect(m_startButton, &QPushButton::pressed, [this]() { OnStartButtonPressed(); });
	bottomLayout->addWidget(m_startButton);

	m_startButton->setEnabled(false);

	mainLayout->addLayout(bottomLayout);

	setLayout(mainLayout);

	m_statusLabel->setText("<b>Downloading news feed...</b>");

	DownloadNewsFeed();
	DownloadManifest();
}

MainWindow::~MainWindow()
{
}

void MainWindow::DownloadManifest()
{
	QByteArray* buffer = new QByteArray;

	QNetworkRequest request(QUrl("https://utopia.digitalpulsesoftware.net/manifest"));
	QNetworkReply* download = m_network.get(request);
	connect(download, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, download, buffer](QNetworkReply::NetworkError /*code*/) { buffer->clear(); OnManifestDownloadError(download); });
	connect(download, &QNetworkReply::finished, [this, download, buffer]() { download->deleteLater(); OnManifestDownloaded(*buffer); delete buffer; });
	connect(download, &QNetworkReply::readyRead, [this, download, buffer]() { buffer->append(download->readAll()); });
}

void MainWindow::DownloadNewsFeed()
{
	QNetworkRequest request(QUrl("https://utopia.digitalpulsesoftware.net/news"));
	QNetworkReply* download = m_network.get(request);
	connect(download, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, download] (QNetworkReply::NetworkError code) { m_newsWidget->setText("Failed to query news: " + download->errorString()); });
	connect(download, &QNetworkReply::finished, [this, download] () { m_newsWidget->setText(download->readAll()); download->deleteLater(); });
	//connect(download, &QNetworkReply::readyRead, [this, download] () { m_newsWidget->append(download->readAll()); });
}

void MainWindow::OnFileDownloadFinish(QFile* fileOutput)
{
	m_downloadedSize += fileOutput->size();
	delete fileOutput;

	m_currentDownload->deleteLater();
	m_currentDownload = nullptr;

	ProcessDownloadList();
}

void MainWindow::OnFileDownloadError(const QString& fileName)
{
	QString errMessage = "Failed to download " + fileName + ": " + ((m_currentDownload) ? m_currentDownload->errorString() : "Failed to open file");
	if (m_currentDownload)
	{
		m_currentDownload->deleteLater();
		m_currentDownload = nullptr;
	}

	std::cerr << errMessage.toStdString() << std::endl;

	m_statusLabel->setText("<b>" + errMessage + "</b>");
}

void MainWindow::OnManifestDownloaded(const QByteArray& buffer)
{
	QJsonParseError parseError;
	QJsonDocument manifest = QJsonDocument::fromJson(buffer, &parseError);
	if (manifest.isNull())
	{
		m_statusLabel->setText("<b>Manifest parsing error: " + parseError.errorString() + "</b>");
		return;
	}

	// Create directories first
	QJsonObject object = manifest.object();
	for (const QJsonValue& entry : object["Directories"].toArray())
		QDir().mkpath("content/" + entry.toString());

	// And then download files
	m_downloadList.clear();
	m_downloadTotalSize = 0;
	for (const QJsonValue& entry : object["Files"].toArray())
	{
		QJsonObject fileData = entry.toObject();
		QString path = "content/" + fileData["path"].toString();
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
					else
						std::cout << "Outdated or corrupt file: " << path.toStdString() << std::endl;
				}
			}
		}
		else
			std::cout << "Missing file: " << path.toStdString() << std::endl;

		m_downloadList.emplace_back(std::move(path));
		m_downloadedSize = 0;
		m_downloadTotalSize += size;
	}

	m_progressBar->setValue(0);

	m_downloadCounter = 0;

	if (!m_downloadList.empty())
	{
		m_startButton->setText("Update");
		m_startButton->setEnabled(true);
	}
}

void MainWindow::OnManifestDownloadError(QNetworkReply* download)
{
	m_statusLabel->setText("<b>Failed to download manifest</b>");
}

void MainWindow::OnStartButtonPressed()
{
	m_startButton->setEnabled(false);

	if (!m_downloadList.empty())
	{
		if (!m_isUpdating)
			ProcessDownloadList();
	}
	else
	{
		// Play
	}
}

void MainWindow::ProcessDownloadList()
{
	if (m_downloadCounter >= m_downloadList.size())
	{
		m_isUpdating = false;
		m_startButton->setText("Start");
		return;
	}

	m_isUpdating = true;

	const QString& filePath = m_downloadList[m_downloadCounter++];

	m_statusLabel->setText("<b> Downloading " + filePath + " (" + QString::number(m_downloadCounter) + '/' + QString::number(m_downloadList.size()) +")</b>");

	QFile* outputFile = new QFile(filePath);
	if (!outputFile->open(QIODevice::WriteOnly))
	{
		OnFileDownloadError(filePath);
		return;
	}

	QNetworkRequest request { QUrl("https://utopia.digitalpulsesoftware.net/" + filePath) };
	std::cout << "Downloading from " + request.url().url().toStdString() << std::endl;

	m_currentDownload = m_network.get(request);
	connect(m_currentDownload, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, filePath] (QNetworkReply::NetworkError /*code*/) { OnFileDownloadError(filePath); });
	connect(m_currentDownload, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) { UpdateProgressBar(bytesTotal, bytesReceived); });
	connect(m_currentDownload, &QNetworkReply::finished, [this, outputFile]() { OnFileDownloadFinish(outputFile); });
	connect(m_currentDownload, &QNetworkReply::readyRead, [this, outputFile] () { outputFile->write(m_currentDownload->readAll()); });
}

void MainWindow::UpdateProgressBar(qint64 /*bytesTotal*/, qint64 bytesReceived)
{
	m_progressBar->setValue(100 * (m_downloadedSize + bytesReceived) / std::max(m_downloadTotalSize, qint64(1)));
}

//#include <Application/Widgets/MainWindow.moc>
