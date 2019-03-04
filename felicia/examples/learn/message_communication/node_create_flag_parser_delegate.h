#ifndef FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_NODE_CREATE_PARSER_DELEGATE_H_
#define FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_NODE_CREATE_PARSER_DELEGATE_H_

#include <memory>

#include "third_party/chromium/base/macros.h"

#include "felicia/core/util/command_line_interface/flag.h"

namespace felicia {

class NodeCreateFlagParserDelegate : public FlagParser::Delegate {
 public:
  NodeCreateFlagParserDelegate();
  ~NodeCreateFlagParserDelegate();

  const std::string& name() const { return name_; }
  const std::string& topic() const { return topic_; }
  const std::string& channel_type() const { return channel_type_; }

  bool Parse(FlagParser& parser) override;

  bool Validate() const override;

  AUTO_DEFINE_USAGE_AND_HELP_TEXT_METHODS(name_flag_, topic_flag_,
                                          channel_type_flag_)

 private:
  std::string name_;
  std::string topic_;
  std::string channel_type_;
  std::unique_ptr<StringFlag> name_flag_;
  std::unique_ptr<StringFlag> topic_flag_;
  std::unique_ptr<StringChoicesFlag> channel_type_flag_;

  DISALLOW_COPY_AND_ASSIGN(NodeCreateFlagParserDelegate);
};

}  // namespace felicia

#endif  // FELICIA_EXAMPLES_LEARN_MESSAGE_COMMUNICATION_NODE_CREATE_PARSER_DELEGATE_H_