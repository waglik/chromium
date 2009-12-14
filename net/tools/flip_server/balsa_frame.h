// Copyright (c) 2009 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_FLIP_SERVER_BALSA_FRAME_H_
#define NET_TOOLS_FLIP_SERVER_BALSA_FRAME_H_

#include <strings.h>

#include <string>
#include <utility>
#include <vector>

#include "base/port.h"
#include "net/tools/flip_server/balsa_enums.h"
#include "net/tools/flip_server/balsa_headers.h"
#include "net/tools/flip_server/balsa_visitor_interface.h"
#include "net/tools/flip_server/buffer_interface.h"
#include "net/tools/flip_server/http_message_constants.h"
#include "net/tools/flip_server/simple_buffer.h"

// For additional debug output, uncomment the following:
// #define DEBUGFRAMER 1

namespace net {

// BalsaFrame is a 'Model' of a framer (haha).
// It exists as a proof of concept headers framer.
class BalsaFrame {
 public:
  typedef vector<pair<size_t, size_t> > Lines;

  typedef BalsaHeaders::HeaderLineDescription HeaderLineDescription;
  typedef BalsaHeaders::HeaderLines HeaderLines;
  typedef BalsaHeaders::HeaderTokenList HeaderTokenList;

  // TODO(fenix): get rid of the 'kValidTerm*' stuff by using the 'since last
  // index' strategy.  Note that this implies getting rid of the HeaderFramed()

  static const int32 kValidTerm1  = '\n' << 16 |
                                    '\r' <<  8 |
                                    '\n';
  static const int32 kValidTerm1Mask = 0xFF << 16 |
                                       0xFF <<  8 |
                                       0xFF;
  static const int32 kValidTerm2      = '\n' << 8 |
                                        '\n';
  static const int32 kValidTerm2Mask = 0xFF << 8 |
                                       0xFF;
  BalsaFrame() :
      last_char_was_slash_r_(false),
      saw_non_newline_char_(false),
      start_was_space_(true),
      chunk_length_character_extracted_(false),
      is_request_(true),
      request_was_head_(false),
      max_header_length_(16 * 1024),
      max_request_uri_length_(2048),
      visitor_(&do_nothing_visitor_),
      chunk_length_remaining_(0),
      content_length_remaining_(0),
      last_slash_n_loc_(NULL),
      last_recorded_slash_n_loc_(NULL),
      last_slash_n_idx_(0),
      term_chars_(0),
      parse_state_(BalsaFrameEnums::READING_HEADER_AND_FIRSTLINE),
      last_error_(BalsaFrameEnums::NO_ERROR),
      headers_(NULL) {}

  ~BalsaFrame() {}

  // Reset reinitializes all the member variables of the framer and clears the
  // attached header object (but doesn't change the pointer value headers_).
  void Reset();

  const BalsaHeaders* const_balsa_headers() const { return headers_; }
  BalsaHeaders* balsa_headers() { return headers_; }
  // The method set_balsa_headers clears the headers provided and attaches them
  // to the framer.  This is a required step before the framer will process any
  // input message data.
  // To detach the header object from the framer, use set_balsa_headers(NULL).
  void set_balsa_headers(BalsaHeaders* headers) {
    if (headers_ != headers) {
      headers_ = headers;
    }
    if (headers_) {
      // Clear the headers if they are non-null, even if the new headers are
      // the same as the old.
      headers_->Clear();
    }
  }

  void set_balsa_visitor(BalsaVisitorInterface* visitor) {
    visitor_ = visitor;
    if (visitor_ == NULL) {
      visitor_ = &do_nothing_visitor_;
    }
  }

  void set_is_request(bool is_request) { is_request_ = is_request; }

  bool is_request() const {
    return is_request_;
  }

  void set_request_was_head(bool request_was_head) {
    request_was_head_ = request_was_head;
  }

  bool request_was_head() const {
    return request_was_head_;
  }

  void set_max_header_length(size_t max_header_length) {
    max_header_length_ = max_header_length;
  }

  size_t max_header_length() const {
    return max_header_length_;
  }

  void set_max_request_uri_length(size_t max_request_uri_length) {
    max_request_uri_length_ = max_request_uri_length;
  }

  size_t max_request_uri_length() const {
    return max_request_uri_length_;
  }


  bool MessageFullyRead() {
    return parse_state_ == BalsaFrameEnums::MESSAGE_FULLY_READ;
  }

  BalsaFrameEnums::ParseState ParseState() const { return parse_state_; }


  bool Error() {
    return parse_state_ == BalsaFrameEnums::ERROR;
  }

  BalsaFrameEnums::ErrorCode ErrorCode() const { return last_error_; }

  const BalsaHeaders* headers() const { return headers_; }
  BalsaHeaders* mutable_headers() { return headers_; }

  size_t BytesSafeToSplice() const;
  void BytesSpliced(size_t bytes_spliced);

  size_t ProcessInput(const char* input, size_t size);

  // Parses input and puts the key, value chunk extensions into extensions.
  // TODO(phython): Find a better data structure to put the extensions into.
  static void ProcessChunkExtensions(const char* input, size_t size,
                                     BalsaHeaders* extensions);

 protected:
  // The utils object needs access to the ParseTokenList in order to do its
  // job.
  friend class BalsaHeadersTokenUtils;

  inline void ProcessContentLengthLine(
      size_t line_idx,
      BalsaHeadersEnums::ContentLengthStatus* status,
      size_t* length);

  inline void ProcessTransferEncodingLine(size_t line_idx);

  void ProcessFirstLine(const char* begin,
                        const char* end);

  void CleanUpKeyValueWhitespace(
      const char* stream_begin,
      const char* line_begin,
      const char* current,
      const char* line_end,
      HeaderLineDescription* current_header_line);

  void FindColonsAndParseIntoKeyValue();

  void ProcessHeaderLines();

  inline size_t ProcessHeaders(const char* message_start,
                               size_t message_length);

  void AssignParseStateAfterHeadersHaveBeenParsed();

  inline bool LineFramingFound(char current_char) {
    return current_char == '\n';
  }

  // TODO(fenix): get rid of the following function and its uses (and
  // replace with something more efficient)
  inline bool HeaderFramingFound(char current_char) {
    // Note that the 'if (current_char == '\n' ...)' test exists to ensure that
    // the HeaderFramingMayBeFound test works properly. In benchmarking done on
    // 2/13/2008, the 'if' actually speeds up performance of the function
    // anyway..
    if (current_char == '\n' || current_char == '\r') {
      term_chars_ <<= 8;
      // This is necessary IFF architecture has > 8 bit char.  Alas, I'm
      // paranoid.
      term_chars_ |= current_char & 0xFF;

      if ((term_chars_ & kValidTerm1Mask) == kValidTerm1) {
        term_chars_ = 0;
        return true;
      }
      if ((term_chars_ & kValidTerm2Mask) == kValidTerm2) {
        term_chars_ = 0;
        return true;
      }
    } else {
      term_chars_ = 0;
    }
    return false;
  }

  inline bool HeaderFramingMayBeFound() const {
    return term_chars_ != 0;
  }

 private:
  class DoNothingBalsaVisitor : public BalsaVisitorInterface {
    virtual void ProcessBodyInput(const char *input, size_t size) {}
    virtual void ProcessBodyData(const char *input, size_t size) {}
    virtual void ProcessHeaderInput(const char *input, size_t size) {}
    virtual void ProcessTrailerInput(const char *input, size_t size) {}
    virtual void ProcessHeaders(const BalsaHeaders& headers) {}
    virtual void ProcessRequestFirstLine(const char* line_input,
                                         size_t line_length,
                                         const char* method_input,
                                         size_t method_length,
                                         const char* request_uri_input,
                                         size_t request_uri_length,
                                         const char* version_input,
                                         size_t version_length) {}
    virtual void ProcessResponseFirstLine(const char *line_input,
                                          size_t line_length,
                                          const char *version_input,
                                          size_t version_length,
                                          const char *status_input,
                                          size_t status_length,
                                          const char *reason_input,
                                          size_t reason_length) {}
    virtual void ProcessChunkLength(size_t chunk_length) {}
    virtual void ProcessChunkExtensions(const char *input, size_t size) {}
    virtual void HeaderDone() {}
    virtual void MessageDone() {}
    virtual void HandleHeaderError(BalsaFrame* framer) {}
    virtual void HandleHeaderWarning(BalsaFrame* framer) {}
    virtual void HandleChunkingError(BalsaFrame* framer) {}
    virtual void HandleBodyError(BalsaFrame* framer) {}
  };

  bool last_char_was_slash_r_;
  bool saw_non_newline_char_;
  bool start_was_space_;
  bool chunk_length_character_extracted_;
  bool is_request_;                // This is not reset in Reset()
  bool request_was_head_;          // This is not reset in Reset()
  size_t max_header_length_;       // This is not reset in Reset()
  size_t max_request_uri_length_;  // This is not reset in Reset()
  BalsaVisitorInterface* visitor_;
  size_t chunk_length_remaining_;
  size_t content_length_remaining_;
  const char* last_slash_n_loc_;
  const char* last_recorded_slash_n_loc_;
  size_t last_slash_n_idx_;
  uint32 term_chars_;
  BalsaFrameEnums::ParseState parse_state_;
  BalsaFrameEnums::ErrorCode last_error_;

  Lines lines_;

  BalsaHeaders* headers_;  // This is not reset to NULL in Reset().
  DoNothingBalsaVisitor do_nothing_visitor_;
};

}  // namespace net

#endif  // NET_TOOLS_FLIP_SERVER_BALSA_FRAME_H_

