//
//           DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//                   Version 2, December 2004
//
// Copyright (C) 2016 Massimo Gengarelli <massimo.gengarelli@gmail.com>
//
// Everyone is permitted to copy and distribute verbatim or modified
// copies of this license document, and changing it is allowed as long
// as the name is changed.
//
//            DO WHAT THE FUCK YOU WANT TO PUBLIC LICENSE
//   TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION
//
//  0. You just DO WHAT THE FUCK YOU WANT TO.
//

#ifndef game_h
#define game_h

#include <stdio.h>
#include <vector>
#include <string>

namespace bgg_client {
namespace data {

class game
{
public:
  game() {};
  game(uint32_t id) : m_gameId(id) {};
  virtual ~game() {};

  // Setters
  void setGameId(const uint32_t& id) { m_gameId = id; }
  void setMinPlayers(const uint32_t& min) { m_minPlayers = min; }
  void setMaxPlayers(const uint32_t& max) { m_maxPlayers = max; }
  void setPlayingTime(const uint32_t& t) { m_playingTime = t; }
  void setYearPublished(const uint32_t& year) { m_yearPublished = year; }
  void setRank(const uint32_t& rank) { m_rank = rank; }

  void setIsExtension(bool const& ext) { m_isExtension = ext; }

  void setGameName(std::string const& name) { m_gameName = name; }
  void setThumbnailUrl(std::string const& url) { m_thumbnailUrl = url; }

  void setAuthor(std::string const& author) { m_authors.push_back(author); }
  void setDescription(std::string const& description) { m_description = description; }

  void setExpands(uint32_t const & id) { m_expands = id; }

  // Getters
  uint32_t const & getGameId() const { return m_gameId; }
  uint32_t const & getMinPlayers () const { return m_minPlayers; }
  uint32_t const & getMaxPlayers () const { return m_maxPlayers; }
  uint32_t const & getPlayingTime () const { return m_playingTime; }
  uint32_t const & getYearPublished () const { return m_yearPublished; }
  uint32_t const & getRank () const { return m_rank; }

  bool const & isExtension() const { return m_isExtension; }

  std::string const & getGameName() const { return m_gameName; }
  std::string const & getThumbnailUrl() const { return m_thumbnailUrl; }

  std::vector<std::string> const & getAuthors() const { return m_authors; }
  std::string const& getDescription() const { return m_description; }

  uint32_t const & getExpands() const { return m_expands; }

  // Needed to create a map.
  bool operator< (bgg_client::data::game const & right) const { return m_gameId < right.m_gameId; }

private:
  uint32_t m_gameId;
  uint32_t m_minPlayers;
  uint32_t m_maxPlayers;
  uint32_t m_playingTime;
  uint32_t m_yearPublished;
  uint32_t m_rank;

  bool m_isExtension;

  std::string m_gameName;
  std::string m_thumbnailUrl;

  std::vector<std::string> m_authors;
  std::string m_description;

  /* Expands game id */
  uint32_t m_expands;

protected:
};

}}

#endif /* game_hpp */
