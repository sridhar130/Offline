#include "Offline/DbTables/inc/DbUtil.hh"
#include "Offline/DbTables/inc/DbIoV.hh"
#include "Offline/DbTables/inc/DbTableFactory.hh"
#include "cetlib_except/exception.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// ****************************************************************
//   read a set of calibration tables from a file
//   format:
// TABLE tableName
// row1_col1 , row1_col2
// row2_col1 , row2_col2
// which may be repeated for more tables
//   the table line may have IOV information (see wiki for formats):
// TABLE tablename start_run:start_sr-end_run:end_sr
// # can include comment lines with hash as first char
mu2e::DbTableCollection mu2e::DbUtil::readFile(std::string const& fn,
                                               bool saveCsv) {
  if (fn.size() <= 0) {
    throw cet::exception("DBFILE_NO_FILE_NAME")
        << "DbUtil::read called with no file name\n";
  }
  std::ifstream myfile;
  myfile.open(fn);
  if (!myfile.is_open()) {
    throw cet::exception("DBFILE_OPEN_FAILED")
        << "DbUtil::read failed to open " << fn << "\n";
  }

  mu2e::DbTableCollection coll;
  mu2e::DbTable::ptr_t current;
  mu2e::DbIoV iov;
  std::vector<mu2e::DbIoV> iovv;
  std::string line, csv;
  int ncom0 = -1, ncom = 0;
  while (std::getline(myfile, line)) {
    boost::trim(line);  // remove whitespace
    // line was blank or comment, skip to next
    if (line.size() <= 0 || line[0] == '#') continue;

    // if this line starts a new table
    if (line.substr(0, 5) == "TABLE") {
      // if there is a table currently being read in, then finish it.
      // first fill the table based on the csv string
      // then add it to the vector as a DbLiveTable
      if (current.get() != nullptr) {
        current->fill(csv, saveCsv);
        for (auto const& ii : iovv) {
          coll.emplace_back(ii, current, -1, -1);
        }
        iovv.clear();
      }

      // split the line into simple words
      std::vector<std::string> words;
      boost::split(words, line, boost::is_any_of(" \t"),
                   boost::token_compress_on);

      if (words.size() < 2) {
        throw cet::exception("DBFILE_NO_TABLE_NAME")
            << "DbUtil::read failed to find table name in:" << line << " \n";
      }

      // create the type of table indicated, in a shared_ptr
      current = mu2e::DbTableFactory::newTable(words[1]);
      csv.clear();
      ncom0 = -1;

      if (words.size() == 2) {  // no iov, assume IOV is all data
        iov.setMax();
        iovv.emplace_back(iov);
      } else {  // has iov run/subrun information
        for (size_t i = 2; i < words.size(); i++) {
          iov.setByString(words[i]);
          iovv.emplace_back(iov);
        }
      }

    } else {
      if (current.get() == nullptr) {  // first line was not a "TABLE" line
        throw cet::exception("DBFILE_BAD_FIRST_LINE")
            << "DbUtil:: First useable line of file was not TABLE : " << line
            << " \n";
      }
      // add the line to the table csv string
      csv.append(line);
      csv.append("\n");
      // check if all csv rows have the same number of fields
      ncom = splitCsv(line).size();
      if (ncom0 < 0) {
        ncom0 = ncom;
      } else {
        if (ncom != ncom0) {
          throw cet::exception("DBFILE_BAD_COMMA_COUNT")
              << "DbUtil::read counted inconsistent number of comma fields in "
                 "line: "
              << line << " \n";
        }
      }
    }
  }
  myfile.close();

  // if there is a table currently being read in, then finish it.
  // first fill the table based on the csv string
  // then add it to the vector as a DbLiveTable
  if (current.get() != nullptr) {
    current->fill(csv, saveCsv);
    for (auto const& ii : iovv) {
      coll.emplace_back(ii, current, -1, -1);
    }
    iovv.clear();
  }

  return coll;
}

// ****************************************************************
// write a set of calibration tables to a file
// output will respect the input format for easy read-edit-commit
void mu2e::DbUtil::writeFile(std::string const& fn,
                             DbTableCollection const& coll) {
  if (fn.size() <= 0) {
    throw cet::exception("DBFILE_NO_FILE_NAME")
        << "DbUtil::write called with no file name\n";
  }
  // silently succeed if nothing to write
  if (coll.size() <= 0) return;

  std::ofstream myfile;
  myfile.open(fn);
  if (!myfile.is_open()) {
    throw cet::exception("DBFILE_OPEN_FAILED")
        << "DbUtil::write failed to open " << fn << "\n";
  }

  // will need some logic so in the case of one cid
  // being used in two iovs, we only write table content once,
  // and write both iovs on one line with the content
  std::vector<bool> dv(coll.size(), false);

  for (size_t i = 0; i < coll.size(); i++) {
    if (!dv[i]) {
      auto const& livet = coll[i];
      DbTable const& tt = livet.table();
      myfile << "TABLE " << tt.name();
      // iov's for all occurances of the i'th cid
      for (size_t j = i; j < coll.size(); j++) {
        auto const& livetj = coll[j];
        if (livetj.cid() == livet.cid()) {
          myfile << " " << livetj.iov().to_string(true);
          dv[j] = true;
        }
      }
      myfile << std::endl;
      // error if table has rows but no content
      // must allow for variable-length tables with no rows
      if (tt.nrow() > 0 && tt.csv().size() == 0) {
        throw cet::exception("DBWRITEFILE_NO_CONTENT")
            << "DbUtil::writeFile no content in table " << tt.name()
            << " with cid " << livet.cid() << "\n";
      }
      myfile << tt.csv();
    }
  }
  myfile.close();
}

// ****************************************************************
// split a big string by its newlines
// the database csv should have a newline at the end of the last line
std::vector<std::string> mu2e::DbUtil::splitCsvLines(const std::string& csv) {
  std::vector<std::string> lines;
  boost::split(lines, csv, boost::is_any_of("\n"), boost::token_compress_off);
  if (!lines.back().empty()) {  // the last \n leads to a blank line at the end
    throw cet::exception("DBUTIL_NO_TERMINAL_NEWLINE")
        << "DbUtil::splitCsvLines csv did not have terminal newline\n";
  }
  lines.pop_back();  // remove empty line
  return lines;
}

// ****************************************************************
// split a line by csv rules, including quotes
// assumes comment lines have been removed
std::vector<std::string> mu2e::DbUtil::splitCsv(const std::string& line) {
  std::vector<std::string> columns;  // columns of a line of csv

  std::size_t i, j;
  i = j = 0;  // i=beginning of current column, j=current position in parse
  bool quote = false;        // deal with commas in quotes
  while (i < line.size()) {  // while not the end of the string
    if (line[j] == '"') {
      if (!quote) {
        quote = true;
        j++;
      } else {                      // could be embedded quote
        if (line[j - 1] == '\\') {  // has form \"
          j++;                      // just continue, slash already processed
        } else if (j < line.size() - 1 && line[j + 1] == '"') {  // has form ""
          j = j + 2;  // just continue, skip second quote
        } else {      // must be end quote
          quote = false;
          j++;
        }
      }
    } else if (line[j] == ',') {  // commas separate the columns
      if (quote) {                // if the comma was in a quote, then skip it
        j++;
      } else {  // non-quoted comma, make new column
        columns.emplace_back(line.c_str() + i, j - i);
        j++;
        i = j;
      }
    } else {  // just a char in a column
      j++;
    }
    if (j == line.size()) {
      if (quote) {  // if still in quote, then error
        throw cet::exception("DBUTIL_OPEN_QUOTE")
            << "DbUtil::splitCsv open quotes at end of line:" + line + "\n";
      }
      columns.emplace_back(line.c_str() + i, j - i);  // get the last column
      i = j;
    }
  }

  for (auto c : columns) boost::trim(c);  // remove lead and trail whitespace

  return columns;
}

// ****************************************************************
// prepare a line of csv for an SQL insert command
// check special characters and add single quotes around every item
std::string mu2e::DbUtil::sqlLine(std::string const& line) {
  auto columns = DbUtil::splitCsv(line);
  std::string clean;
  for (auto cc : columns) {
    // if it is quoted, switch to single quotes
    // it should be trimmed of whitespace
    if (cc.front() == '"' && cc.back() == '"') {
      cc.front() = '\'';
      cc.back() = '\'';
    } else {
      // put it in single quotes
      cc = "'" + cc + "'";
    }
    // if there is a '""' or '\"', change them to '"'
    // if there is a ', change it to ''
    auto it = cc.begin();
    it++;  // skip first quote
    while (it != (cc.end() - 1)) {
      if ((*it) == '\\' && (*(it + 1)) == '"') {
        cc.erase(it);
      }
      if ((*it) == '"' && (*(it + 1)) == '"') {
        cc.erase(it);
      }
      if ((*it) == '\'') {
        cc.insert(it, '\'');
        it++;
      }
      it++;
    }
    clean.append(cc + ",");
  }
  clean.pop_back();  // remove last comma
  return clean;
}

// ****************************************************************
// return current local time as a formatted string
std::string mu2e::DbUtil::timeString() {
  time_t mtime;
  time(&mtime);
  struct tm* mtm = localtime(&mtime);
  std::ostringstream ss;
  ss << std::setfill('0') << std::setw(2) << mtm->tm_mon + 1 << "/"
     << std::setw(2) << mtm->tm_mday << "/" << std::setw(2)
     << mtm->tm_year % 100 << " " << std::setw(2) << mtm->tm_hour << ":"
     << std::setw(2) << mtm->tm_min << ":" << std::setw(2) << mtm->tm_sec;
  return ss.str();
}
