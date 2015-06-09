/**
 * This file is part of the "libfnord" project
 *   Copyright (c) 2015 Paul Asmuth
 *
 * FnordMetric is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License v3.0. You should have received a
 * copy of the GNU General Public License along with this program. If not, see
 * <http://www.gnu.org/licenses/>.
 */
#ifndef _FNORD_MSG_MESSAGESCHEMA_H
#define _FNORD_MSG_MESSAGESCHEMA_H
#include <fnord-base/stdtypes.h>
#include <fnord-base/exception.h>
#include <fnord-base/autoref.h>
#include <fnord-msg/MessageObject.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>

/**
 * // http://tools.ietf.org/html/rfc5234
 *
 *   <message> := <object>
 *
 *   <message> :=
 *       <varint>               // num fields
 *       { <field_ptr> }        // one field pointer for each field
 *       { <uint8_t> }          // field data
 *
 *   <field_ptr> :=
 *       <varint>               // field id
 *       <varint>               // field data end offset
 *
 */
namespace fnord {
namespace msg {

enum class EncodingHint : uint8_t {
  NONE = 0,
  BITPACK = 1,
  LEB128 = 2
};

class MessageSchema;

struct MessageSchemaField {

  static MessageSchemaField mkObjectField(
      uint32_t id,
      String name,
      bool repeated,
      bool optional,
      RefPtr<msg::MessageSchema> schema);

  MessageSchemaField(
    uint32_t _id,
    String _name,
    FieldType _type,
    uint64_t _type_size,
    bool _repeated,
    bool _optional,
    EncodingHint _encoding = EncodingHint::NONE) :
    id(_id),
    name(_name),
    type(_type),
    type_size(_type_size),
    repeated(_repeated),
    optional(_optional),
    encoding(_encoding),
    schema(nullptr) {}

  uint32_t id;
  String name;
  FieldType type;
  uint64_t type_size;
  bool repeated;
  bool optional;
  EncodingHint encoding;
  RefPtr<MessageSchema> schema;
};

class MessageSchema : public RefCounted {
public:

  static RefPtr<MessageSchema> fromProtobuf(
      const google::protobuf::Descriptor* dsc);

  MessageSchema(
      const String& name,
      Vector<MessageSchemaField> fields);

  MessageSchema(const MessageSchema& other);

  const String& name() const;

  const Vector<MessageSchemaField>& fields() const;
  uint32_t fieldId(const String& name) const;
  FieldType fieldType(uint32_t id) const;
  const String& fieldName(uint32_t id) const;
  RefPtr<MessageSchema> fieldSchema(uint32_t id) const;

  Set<String> columns() const;
  String toString() const;

protected:
  String name_;
  Vector<MessageSchemaField> fields_;
  HashMap<String, uint32_t> field_ids_;
  HashMap<uint32_t, FieldType> field_types_;
  HashMap<uint32_t, String> field_names_;
};

class MessageSchemaRepository {
public:

  RefPtr<MessageSchema> getSchema(const String& name) const;

  void registerSchema(RefPtr<MessageSchema> schema);

protected:
  HashMap<String, RefPtr<MessageSchema>> schemas_;
};

} // namespace msg
} // namespace fnord

#endif
