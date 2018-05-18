// Copyright (C) 2017 Full Cycle Games
// This file is part of the "Load'n'Rush Server Console" project
// For conditions of distribution and use, see copyright notice in COPYRIGHT

#include <Launcher/Widgets/MainWindow.hpp>
#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QProcess>
#include <QtNetwork/QNetworkReply>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QVBoxLayout>
#include <iostream>

MainWindow::MainWindow()
{
	resize(1024, 768);
	setWindowTitle("Utopia Launcher v0.01");

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
	m_progressBar->hide();

	m_startButton = new QPushButton("Play");
	connect(m_startButton, &QPushButton::pressed, [this]() { OnStartButtonPressed(); });
	bottomLayout->addWidget(m_startButton);

	m_startButton->setEnabled(false);

	mainLayout->addLayout(bottomLayout);

	setLayout(mainLayout);

	m_statusLabel->setText("<b>Downloading manifest...</b>");

	DownloadNewsFeed();
	DownloadManifest();
}

MainWindow::~MainWindow()
{
}

void MainWindow::AddToDownloadList(QJsonObject manifest, const QString& outputFolder)
{
	AddToDownloadList(manifest, outputFolder, outputFolder);
}

void MainWindow::AddToDownloadList(QJsonObject manifest, const QString& outputFolder, const QString& referenceFolder)
{
	QString downloadFolder = manifest["DownloadFolder"].toString();

	// And then download files
	std::size_t fileIndex = 0;
	for (const QJsonValue& entry : manifest["Files"].toArray())
	{
		QJsonObject fileData = entry.toObject();
		QString downloadPath = fileData["downloadPath"].toString();
		QString targetPath = fileData["targetPath"].toString();
		bool isProtected = fileData["protected"].toBool(false);
		int size = fileData["size"].toInt();
		QString hash = fileData["hash"].toString();

		QString referencePath = referenceFolder + '/' + targetPath;

		if (QFile::exists(referencePath))
		{
			if (isProtected)
				continue;

			QFile file(referencePath);
			if (!file.open(QIODevice::ReadOnly))
			{
				std::cerr << "Failed to open: " << referencePath.toStdString() << std::endl;
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
						std::cout << "Detected outdated or corrupt file: " << referencePath.toStdString() << std::endl;
				}
			}
			else
				std::cout << "Detected outdated or corrupt file: " << referencePath.toStdString() << std::endl;
		}
		else
			std::cout << "Detected missing file: " << referencePath.toStdString() << std::endl;

		auto& downloadEntry = m_downloadList.emplace_back();
		downloadEntry.baseName = targetPath;
		downloadEntry.downloadUrl = downloadPath;
		downloadEntry.outputFile = outputFolder + '/' + targetPath;

		m_downloadTotalSize += size;
	}
}

void MainWindow::DownloadManifest()
{
	QByteArray* buffer = new QByteArray;

	QString manifestName = "manifest";
#if defined(Q_OS_WIN)
	manifestName += ".windows";
#elif defined(Q_OS_LINUX)
	manifestName += ".linux";
#endif

	QNetworkRequest request(QUrl("https://utopia.digitalpulsesoftware.net/" + manifestName));
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

void MainWindow::OnFileDownloadError(QNetworkReply* download, const QString& fileName)
{
	m_downloadCounter--;

	QString errMessage = "Failed to download " + fileName + ": " + ((download) ? download->errorString() : "Failed to open file");

	m_statusLabel->setText("<b>" + errMessage + "</b>");

	m_startButton->setText("Try again");
	m_startButton->setEnabled(true);
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

	m_downloadList.clear();
	m_downloadTotalSize = 0;

	QJsonObject manifestObject = manifest.object();

	AddToDownloadList(manifestObject["Launcher"].toObject(), "tmp", ".");

	// Only download game files when launcher is updated
	m_isUpdatingLauncher = !m_downloadList.empty();

	if (!m_isUpdatingLauncher)
		AddToDownloadList(manifestObject["Game"].toObject(), "game");

	if (!m_downloadList.empty())
	{
		if (m_isUpdatingLauncher)
			m_statusLabel->setText("<b>A new launcher update is available</b>");
		else
			m_statusLabel->setText("<b>A new game update is available</b>");

		m_downloadCounter = 0;

		m_startButton->setText("Update");
		m_startButton->setEnabled(true);
	}
	else
		SetupPlayButton();
}

void MainWindow::OnManifestDownloadError(QNetworkReply* download)
{
	m_statusLabel->setText("<b>Failed to download manifest</b>");
}

void MainWindow::OnStartButtonPressed()
{
	if (!m_downloadList.empty())
	{
		m_startButton->setEnabled(false);

		m_downloadedSize = 0;
		m_progressBar->setValue(0);
		m_progressBar->show();

		ProcessDownloadList();
	}
	else if (m_isUpdatingLauncher)
	{
#if defined(Q_OS_WIN)
		QFile cmdFile("updateLauncher.bat");
#elif defined(Q_OS_LINUX)
		QFile cmdFile("updateLauncher.sh");
#else
#error "Unsupported OS"
#endif

		if (!cmdFile.open(QIODevice::WriteOnly))
		{
			m_statusLabel->setText("Failed to open " + cmdFile.fileName());
			m_statusLabel->show();
			return;
		}

		QTextStream outputStream(&cmdFile);

#if defined(Q_OS_WIN)
		QString executable = cmdFile.fileName();
		QStringList parameters = {};

		outputStream << "echo \"Waiting for launcher to close\"" << "\r\n";
		outputStream << "\r\n";
		outputStream << ":loop" << "\r\n";
		outputStream << "tasklist | find \" " << qApp->applicationPid() << " \" > nul" << "\r\n";
		outputStream << "if not errorlevel 1 (" << "\r\n";
		outputStream << "\t" << "timeout /t 1 > nul" << "\r\n";
		outputStream << "\t" << "goto :loop" << "\r\n";
		outputStream << ")" << "\r\n";
		outputStream << "\r\n";
		outputStream << "robocopy /E /MOVE tmp ." << "\r\n";
		outputStream << R"(start "" "ErewhonLauncher.exe")" << "\r\n";
#elif defined(Q_OS_LINUX)
		QString executable = "/bin/sh";
		QStringList parameters = { cmdFile.fileName() };

		outputStream << "#!/bin/bash\n";
		outputStream << "\n";
		outputStream << "echo \"Waiting for launcher to close\"";
		outputStream << "while ps -p " << qApp->applicationPid() << " > /dev/null; do sleep 1; done;";
		outputStream << "\n";
		outputStream << "./ErewhonLauncher";
#else
#error "Unsupported OS"
#endif

		cmdFile.setPermissions(cmdFile.permissions() | QFileDevice::ExeUser);

		cmdFile.close();

		if (QProcess::startDetached(executable, parameters))
		{
			qApp->quit();
		}
		else
		{
			m_statusLabel->setText("Failed to start " + cmdFile.fileName() + ", please close the launcher and execute it");
			m_statusLabel->show();
			return;
		}
	}
	else
	{
		// Play
		QMessageBox::warning(this, "Not yet implemented", "Play button is not yet implemented, launch ErewhonClient.exe in the game folder");
	}
}

void MainWindow::ProcessDownloadList()
{
	if (m_downloadCounter >= m_downloadList.size())
	{
		m_downloadList.clear();

		if (m_isUpdatingLauncher)
		{
			m_progressBar->hide();

			m_statusLabel->setText("<b>Launcher update has been downloaded</b>");
			m_statusLabel->show();

			m_startButton->setText("Restart launcher");
			m_startButton->setEnabled(true);
		}
		else
			SetupPlayButton();

		return;
	}

	const auto& downloadEntry = m_downloadList[m_downloadCounter++];

	m_statusLabel->setText("<b> Downloading " + downloadEntry.baseName + " (" + QString::number(m_downloadCounter) + '/' + QString::number(m_downloadList.size()) +")</b>");

	QFileInfo outputFileInfo(downloadEntry.outputFile);
	if (!QDir().mkpath(outputFileInfo.dir().path()))
	{
		OnFileDownloadError(nullptr, outputFileInfo.dir().path());
		return;
	}

	QFile* outputFile = new QFile(downloadEntry.outputFile);
	if (!outputFile->open(QIODevice::WriteOnly))
	{
		OnFileDownloadError(nullptr, downloadEntry.outputFile);
		return;
	}

	QNetworkRequest request { QUrl("https://utopia.digitalpulsesoftware.net/" + downloadEntry.downloadUrl) };
	std::cout << "Downloading from " + request.url().url().toStdString() << std::endl;

	m_currentDownload = m_network.get(request);
	connect(m_currentDownload, (void (QNetworkReply::*)(QNetworkReply::NetworkError)) &QNetworkReply::error, [this, filePath = downloadEntry.baseName] (QNetworkReply::NetworkError /*code*/) { OnFileDownloadError(m_currentDownload, filePath); });
	connect(m_currentDownload, &QNetworkReply::downloadProgress, [this](qint64 bytesReceived, qint64 bytesTotal) { UpdateProgressBar(bytesTotal, bytesReceived); });
	connect(m_currentDownload, &QNetworkReply::finished, [this, outputFile]() { OnFileDownloadFinish(outputFile); });
	connect(m_currentDownload, &QNetworkReply::readyRead, [this, outputFile] () { outputFile->write(m_currentDownload->readAll()); });
}

void MainWindow::SetupPlayButton()
{
	m_progressBar->hide();
	m_statusLabel->hide();

	m_startButton->setText("Play");
	m_startButton->setEnabled(true);
}

void MainWindow::UpdateProgressBar(qint64 /*bytesTotal*/, qint64 bytesReceived)
{
	m_progressBar->setValue(100 * (m_downloadedSize + bytesReceived) / std::max(m_downloadTotalSize, qint64(1)));
}

//#include <Application/Widgets/MainWindow.moc>
