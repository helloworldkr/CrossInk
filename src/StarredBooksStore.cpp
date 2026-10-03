#include "StarredBooksStore.h"

#include <Epub.h>
#include <FsHelpers.h>
#include <HalStorage.h>
#include <Logging.h>
#include <Xtc.h>

#include <algorithm>
#include <utility>

#include "RecentBooksStore.h"

void StarredBooksStore::toJson(JsonDocument& doc) const {
  JsonArray arr = doc["books"].to<JsonArray>();
  for (const auto& book : starredBooks) {
    JsonObject obj = arr.add<JsonObject>();
    obj["path"] = book.path;
    obj["title"] = book.title;
    obj["author"] = book.author;
    obj["coverBmpPath"] = book.coverBmpPath;
  }
}

bool StarredBooksStore::fromJson(JsonVariantConst doc) {
  starredBooks.clear();
  JsonArrayConst arr = doc["books"].as<JsonArrayConst>();
  starredBooks.reserve(std::min(arr.size(), static_cast<size_t>(MAX_STARRED_BOOKS)));
  for (JsonObjectConst obj : arr) {
    if (starredBooks.size() >= static_cast<size_t>(MAX_STARRED_BOOKS)) break;
    StarredBook book;
    book.path = obj["path"] | "";
    book.title = obj["title"] | "";
    book.author = obj["author"] | "";
    book.coverBmpPath = obj["coverBmpPath"] | "";
    if (!book.path.empty()) {
      starredBooks.push_back(std::move(book));
    }
  }
  return true;
}

bool StarredBooksStore::saveToFile() const {
  std::lock_guard<std::mutex> lock(storeMutex);
  JsonDocument doc;
  toJson(doc);
  return PersistableStoreBase::writeDocToFileAtomically(getFilePath(), doc);
}

bool StarredBooksStore::isStarred(const std::string& path) const {
  ensureLoaded();
  std::lock_guard<std::mutex> lock(storeMutex);
  return std::any_of(starredBooks.begin(), starredBooks.end(),
                     [&](const StarredBook& b) { return b.path == path; });
}

bool StarredBooksStore::addStar(const std::string& path, const std::string& title, const std::string& author,
                                const std::string& coverBmpPath) {
  if (path.empty()) return false;
  ensureLoaded();

  std::string finalTitle = title;
  std::string finalAuthor = author;
  std::string finalCover = coverBmpPath;

  if (finalTitle.empty()) {
    RecentBook rb = RECENT_BOOKS.getDataFromBook(path);
    if (!rb.title.empty()) {
      finalTitle = rb.title;
      finalAuthor = rb.author;
      if (finalCover.empty()) finalCover = rb.coverBmpPath;
    } else {
      const size_t lastSlash = path.find_last_of("/\\");
      const std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
      const size_t lastDot = filename.find_last_of('.');
      finalTitle = (lastDot != std::string::npos) ? filename.substr(0, lastDot) : filename;
    }
  }

  {
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = std::find_if(starredBooks.begin(), starredBooks.end(),
                           [&](const StarredBook& b) { return b.path == path; });
    if (it != starredBooks.end()) {
      if (!finalTitle.empty()) it->title = finalTitle;
      if (!finalAuthor.empty()) it->author = finalAuthor;
      if (!finalCover.empty()) it->coverBmpPath = finalCover;
    } else {
      if (starredBooks.size() >= static_cast<size_t>(MAX_STARRED_BOOKS)) {
        starredBooks.pop_back();
      }
      starredBooks.insert(starredBooks.begin(), {path, finalTitle, finalAuthor, finalCover});
    }
  }

  saveToFile();
  return true;
}

bool StarredBooksStore::removeStar(const std::string& path) {
  ensureLoaded();
  bool removed = false;
  {
    std::lock_guard<std::mutex> lock(storeMutex);
    auto it = std::find_if(starredBooks.begin(), starredBooks.end(),
                           [&](const StarredBook& b) { return b.path == path; });
    if (it != starredBooks.end()) {
      starredBooks.erase(it);
      removed = true;
    }
  }
  if (removed) {
    saveToFile();
  }
  return removed;
}

bool StarredBooksStore::toggleStar(const std::string& path, const std::string& title, const std::string& author,
                                   const std::string& coverBmpPath) {
  if (isStarred(path)) {
    removeStar(path);
    return false;
  } else {
    addStar(path, title, author, coverBmpPath);
    return true;
  }
}

bool StarredBooksStore::updatePath(const std::string& oldPath, const std::string& newPath,
                                   const std::string& oldCachePath, const std::string& newCachePath) {
  ensureLoaded();
  bool found = false;
  {
    std::lock_guard<std::mutex> lock(storeMutex);
    for (auto& b : starredBooks) {
      if (b.path == oldPath) {
        b.path = newPath;
        if (!oldCachePath.empty() && !newCachePath.empty() &&
            b.coverBmpPath.rfind(oldCachePath, 0) == 0) {
          b.coverBmpPath = newCachePath + b.coverBmpPath.substr(oldCachePath.length());
        }
        found = true;
        break;
      }
    }
  }
  if (found) {
    saveToFile();
  }
  return found;
}

bool StarredBooksStore::isMissing(const StarredBook& book) {
  return !book.path.empty() && !Storage.exists(book.path.c_str());
}

bool StarredBooksStore::pruneMissing() {
  ensureLoaded();
  std::lock_guard<std::mutex> lock(storeMutex);
  const size_t before = starredBooks.size();
  starredBooks.erase(std::remove_if(starredBooks.begin(), starredBooks.end(),
                                    [](const StarredBook& b) { return isMissing(b); }),
                     starredBooks.end());
  return starredBooks.size() != before;
}

StarredBook StarredBooksStore::getBook(const std::string& path) const {
  ensureLoaded();
  std::lock_guard<std::mutex> lock(storeMutex);
  for (const auto& b : starredBooks) {
    if (b.path == path) return b;
  }
  return {};
}
