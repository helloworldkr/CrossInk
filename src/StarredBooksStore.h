#pragma once
#include <ArduinoJson.h>
#include <PersistableStore.h>

#include <string>
#include <vector>

struct StarredBook {
  std::string path;
  std::string title;
  std::string author;
  std::string coverBmpPath;

  bool operator==(const StarredBook& other) const { return path == other.path; }
};

class StarredBooksStore : public PersistableStore<StarredBooksStore> {
 private:
  std::vector<StarredBook> starredBooks;

  static constexpr int MAX_STARRED_BOOKS = 50;

  StarredBooksStore() = default;
  ~StarredBooksStore() = default;

  friend class PersistableStore<StarredBooksStore>;

 public:
  static const char* getFilePath() { return "/.crosspoint/starred.json"; }
  void toJson(JsonDocument& doc) const;
  bool fromJson(JsonVariantConst doc);
  bool saveToFile() const;

  // Check if a book path is currently starred
  bool isStarred(const std::string& path) const;

  // Toggle star status: returns true if newly starred, false if removed
  bool toggleStar(const std::string& path, const std::string& title = "", const std::string& author = "",
                  const std::string& coverBmpPath = "");

  // Add or update a starred book
  bool addStar(const std::string& path, const std::string& title = "", const std::string& author = "",
               const std::string& coverBmpPath = "");

  // Remove a star by path. Returns true if it was found and removed.
  bool removeStar(const std::string& path);

  // Update book path and cover cache path on rename/move
  [[nodiscard]] bool updatePath(const std::string& oldPath, const std::string& newPath,
                                const std::string& oldCachePath, const std::string& newCachePath);

  static bool isMissing(const StarredBook& book);
  bool pruneMissing();

  const std::vector<StarredBook>& getBooks() const {
    ensureLoaded();
    return starredBooks;
  }

  int getCount() const {
    ensureLoaded();
    return static_cast<int>(starredBooks.size());
  }

  StarredBook getBook(const std::string& path) const;
};

#define STARRED_BOOKS StarredBooksStore::getInstance()
