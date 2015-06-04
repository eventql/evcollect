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

struct MessageSchemaField {

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
    encoding(_encoding) {}

  uint32_t id;
  String name;
  FieldType type;
  uint64_t type_size;
  bool repeated;
  bool optional;
  EncodingHint encoding;
  Vector<MessageSchemaField> fields;
};

struct MessageSchema : public RefCounted {
  MessageSchema(
      const String& _name,
      Vector<MessageSchemaField> _fields);

  MessageSchema(const MessageSchema& other);

  String name_;
  Vector<MessageSchemaField> fields;
  HashMap<String, uint32_t> field_ids;
  HashMap<uint32_t, FieldType> field_types;
  HashMap<uint32_t, String> field_names;

  uint32_t id(const String& path) const;
  FieldType type(uint32_t id) const;
  const String& name(uint32_t id) const;
  Set<String> columns() const;
  String toString() const;
};

} // namespace msg
} // namespace fnord

#endif
