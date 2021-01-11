#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"
#include "halley/support/logger.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project, std::optional<std::set<String>> assetsToPack, std::vector<String> deletedAssets)
	: EditorTask("Packing assets", true, true)
	, project(project)
	, assetsToPack(std::move(assetsToPack))
	, deletedAssets(std::move(deletedAssets))
{
}

void AssetPackerTask::run()
{
	Logger::logInfo("Packing assets (" + toString(assetsToPack->size()) + " modified).");
	AssetPacker::pack(project, assetsToPack, deletedAssets);
	Logger::logInfo("Done packing assets");

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (assetsToPack) {
			Concurrent::execute(Executors::getMainThread(), [project = &project, assets = std::move(assetsToPack)] () {
				project->reloadAssets(assets.value(), true);
			});
		}
	}
}
