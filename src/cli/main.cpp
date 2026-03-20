// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDomDocument>
#include <cstdlib>
#include <iostream>

#include <core/BatchPipeline.h>
#include <core/FileNameDisambiguator.h>
#include <core/FilterResult.h>
#include <core/OutputFileNameGenerator.h>
#include <core/PageSelectionAccessor.h>
#include <core/PageView.h>
#include <core/ProcessingTaskQueue.h>
#include <core/ProjectPages.h>
#include <core/ProjectReader.h>
#include <core/StageSequence.h>
#include <core/ThumbnailPixmapCache.h>
#include <core/Utils.h>
#include <core/WorkerThreadPool.h>
#include "version.h"

#include "CliConfig.h"
#include "CliPageSelectionProvider.h"
#include "NoOpFilterUiInterface.h"
#include "ProjectGeneratorFromDir.h"

static void usage(const char* prog) {
  QTextStream err(stderr);
  err << "Usage: " << prog
      << " [OPTIONS] [PROJECT.ScanTailor]\n"
         "  --from-dir DIR       Create project from images in DIR and process (no GUI)\n"
         "  -c, --config FILE    Load config from FILE (project=, threads=, output-dir=)\n"
         "  -o, --output-dir DIR Override output directory\n"
         "  -t, --threads N      Max parallel threads (default: auto)\n"
         "  -h, --help           Show this help\n"
         "Arguments override config file values.\n";
}

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  QCoreApplication::setApplicationName(QLatin1String("scantailor-cli"));

  QString projectPath;
  QString fromDir;
  QString configPath;
  QString outputDirOverride;
  int threadsOverride = -1;

  for (int i = 1; i < argc; ++i) {
    QString arg = QString::fromUtf8(argv[i]);
    if (arg == QLatin1String("-h") || arg == QLatin1String("--help")) {
      usage(argv[0]);
      return 0;
    }
    if (arg == QLatin1String("--from-dir")) {
      if (i + 1 >= argc) {
        std::cerr << "Missing argument for " << arg.toStdString() << "\n";
        return 1;
      }
      fromDir = QString::fromUtf8(argv[++i]);
      continue;
    }
    if (arg == QLatin1String("-c") || arg == QLatin1String("--config")) {
      if (i + 1 >= argc) {
        std::cerr << "Missing argument for " << arg.toStdString() << "\n";
        return 1;
      }
      configPath = QString::fromUtf8(argv[++i]);
      continue;
    }
    if (arg == QLatin1String("-o") || arg == QLatin1String("--output-dir")) {
      if (i + 1 >= argc) {
        std::cerr << "Missing argument for " << arg.toStdString() << "\n";
        return 1;
      }
      outputDirOverride = QString::fromUtf8(argv[++i]);
      continue;
    }
    if (arg == QLatin1String("-t") || arg == QLatin1String("--threads")) {
      if (i + 1 >= argc) {
        std::cerr << "Missing argument for " << arg.toStdString() << "\n";
        return 1;
      }
      bool ok = false;
      threadsOverride = QString::fromUtf8(argv[++i]).toInt(&ok);
      if (!ok || threadsOverride < 1) {
        std::cerr << "Invalid threads value\n";
        return 1;
      }
      continue;
    }
    if (!arg.startsWith(QLatin1Char('-'))) {
      projectPath = arg;
      break;
    }
    std::cerr << "Unknown option: " << arg.toStdString() << "\n";
    return 1;
  }

  CliConfig config;
  if (!configPath.isEmpty()) {
    config.loadFromFile(configPath);
  } else {
    QDir exeDir(QCoreApplication::applicationDirPath());
    QString localConfig = exeDir.absoluteFilePath(QLatin1String("scantailor-cli.conf"));
    config.loadFromFile(localConfig);
  }

  if (projectPath.isEmpty()) {
    projectPath = config.projectPath;
  }
  if (outputDirOverride.isEmpty()) {
    outputDirOverride = config.outputDir;
  }
  if (threadsOverride < 0) {
    threadsOverride = config.threads;
  }

  if (!fromDir.isEmpty()) {
    const QDir dir(fromDir);
    if (!dir.exists()) {
      std::cerr << "Directory not found: " << fromDir.toStdString() << "\n";
      return 1;
    }
    projectPath = ProjectGeneratorFromDir::generate(dir.absolutePath(), QStringLiteral("out"));
    if (projectPath.isEmpty()) {
      std::cerr << "No images found in directory or failed to create project: " << fromDir.toStdString() << "\n";
      return 1;
    }
    std::cout << "Created project: " << projectPath.toStdString() << "\n";
  }

  if (projectPath.isEmpty()) {
    std::cerr << "No project file specified. Use PROJECT.ScanTailor, --from-dir DIR, or -c config.conf with project=\n";
    return 1;
  }

  QFileInfo projectFileInfo(projectPath);
  if (!projectFileInfo.isFile()) {
    std::cerr << "Project file not found: " << projectPath.toStdString() << "\n";
    return 1;
  }
  QString absoluteProjectPath = projectFileInfo.absoluteFilePath();

  QFile file(absoluteProjectPath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    std::cerr << "Cannot open project file: " << absoluteProjectPath.toStdString() << "\n";
    return 1;
  }
  QDomDocument doc;
  QString errMsg;
  int errLine, errCol;
  if (!doc.setContent(&file, false, &errMsg, &errLine, &errCol)) {
    std::cerr << "Invalid project XML: " << errMsg.toStdString() << " at " << errLine << ":" << errCol << "\n";
    return 1;
  }
  file.close();

  ProjectReader reader(doc, absoluteProjectPath);
  if (!reader.success()) {
    if (!reader.getVersion().isNull() && reader.getVersion().toInt() != PROJECT_VERSION) {
      std::cerr << "Project file is not compatible with this application version.\n";
    } else {
      std::cerr << "Unable to interpret the project file.\n";
    }
    return 1;
  }

  std::shared_ptr<ProjectPages> pages = reader.pages();
  if (!pages->validateDpis()) {
    std::cerr << "Some images have invalid DPI. Fix DPI in the GUI and save the project, then run the CLI again.\n";
    return 1;
  }

  QString outDir = reader.outputDirectory();
  if (!outputDirOverride.isEmpty()) {
    outDir = outputDirOverride;
    if (!QDir::isAbsolutePath(outDir)) {
      outDir = QDir(projectFileInfo.absolutePath()).absoluteFilePath(outDir);
    }
  }
  if (outDir.isEmpty()) {
    std::cerr << "Project has no output directory. Set it in the GUI or use -o.\n";
    return 1;
  }

  std::shared_ptr<FileNameDisambiguator> disambiguator = reader.namingDisambiguator();
  OutputFileNameGenerator outFileNameGen(disambiguator, outDir, pages->layoutDirection());

  const PageSequence imageViewSequence = pages->toPageSequence(IMAGE_VIEW);
  for (const PageInfo& p : imageViewSequence) {
    outFileNameGen.disambiguator()->registerFile(p.imageId().filePath());
  }

  auto selectionProvider = std::make_shared<CliPageSelectionProvider>(pages->toPageSequence(PAGE_VIEW));
  PageSelectionAccessor selectionAccessor(selectionProvider);
  std::shared_ptr<StageSequence> stages = std::make_shared<StageSequence>(pages, selectionAccessor);
  reader.readFilterSettings(stages->filters());

  core::Utils::maybeCreateCacheDir(outDir);
  const QString thumbDir = core::Utils::outputDirToThumbDir(outDir);
  std::shared_ptr<ThumbnailPixmapCache> thumbnailCache =
      std::make_shared<ThumbnailPixmapCache>(thumbDir, QSize(700, 700), 40, 5);

  ProcessingTaskQueue queue;
  const PageSequence batchSequence = pages->toPageSequence(PAGE_VIEW);
  const int lastFilterIdx = stages->outputFilterIdx();
  for (const PageInfo& page : batchSequence) {
    BackgroundTaskPtr task = BatchPipeline::createCompositeTask(
        page, lastFilterIdx, *stages, thumbnailCache, outFileNameGen, pages, true, false);
    queue.addProcessingTask(page, task);
  }

  WorkerThreadPool pool(&app);
  if (threadsOverride > 0) {
    pool.setMaxThreadOverride(threadsOverride);
  }

  NoOpFilterUiInterface noopUi;
  QObject::connect(&pool, &WorkerThreadPool::taskResult, &app, [&queue, &pool, &noopUi](const BackgroundTaskPtr& task,
                                                                                        const FilterResultPtr& result) {
    queue.processingFinished(task);
    if (!task->isCancelled() && result) {
      result->updateUI(&noopUi);
    }
    if (queue.allProcessed()) {
      QCoreApplication::quit();
      return;
    }
    BackgroundTaskPtr next = queue.takeForProcessing();
    while (next) {
      pool.submitTask(next);
      if (!pool.hasSpareCapacity()) {
        break;
      }
      next = queue.takeForProcessing();
    }
  });

  BackgroundTaskPtr first = queue.takeForProcessing();
  if (!first) {
    std::cout << "No pages to process.\n";
    return 0;
  }
  do {
    pool.submitTask(first);
    if (!pool.hasSpareCapacity()) {
      break;
    }
  } while ((first = queue.takeForProcessing()));

  if (queue.allProcessed()) {
    QCoreApplication::quit();
  }

  return app.exec();
}
