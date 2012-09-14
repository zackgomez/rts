import std.json, std.variant, std.conv, std.traits;

Variant jsonToVariant(string json) {
  auto decoded = parseJSON(json);
  return jsonValueToVariant(decoded);
}

Variant jsonValueToVariant(JSONValue v) {
  Variant ret;

  final switch(v.type) {
  case JSON_TYPE.STRING:
    ret = v.str;
    break;
  case JSON_TYPE.INTEGER:
    ret = v.integer;
    break;
  case JSON_TYPE.UINTEGER:
    ret = v.uinteger;
    break;
  case JSON_TYPE.FLOAT:
    ret = v.floating;
    break;
  case JSON_TYPE.OBJECT:
    Variant[string] obj;
    foreach(k, val; v.object) {
      obj[k] = jsonValueToVariant(val);
    }

    ret = obj;
    break;
  case JSON_TYPE.ARRAY:
    Variant[] arr;
    foreach(i; v.array) {
      arr ~= jsonValueToVariant(i);
    }

    ret = arr;
    break;
  case JSON_TYPE.TRUE:
    ret = true;
    break;
  case JSON_TYPE.FALSE:
    ret = false;
    break;
  case JSON_TYPE.NULL:
    ret = null;
    break;
  }

  return ret;
}

string toJson(T)(T a) {
  auto v = toJsonValue(a);
  return toJSON(&v);
}

JSONValue toJsonValue(T)(T a) {
  JSONValue val;
  static if(is(T == JSONValue)) {
    val = a;
  } else static if(__traits(compiles, val = a.makeJsonValue())) {
    val = a.makeJsonValue();
  } else static if(isIntegral!(T)) {
    val.type = JSON_TYPE.INTEGER;
    val.integer = to!long(a);
  } else static if(isFloatingPoint!(T)) {
    val.type = JSON_TYPE.FLOAT;
    val.floating = to!real(a);
    static assert(0);
  } else static if(is(T == void*)) {
    val.type = JSON_TYPE.NULL;
  } else static if(is(T == bool)) {
    if(a == true)
      val.type = JSON_TYPE.TRUE;
    if(a == false)
      val.type = JSON_TYPE.FALSE;
  } else static if(isSomeString!(T)) {
    val.type = JSON_TYPE.STRING;
    val.str = to!string(a);
  } else static if(isAssociativeArray!(T)) {
    val.type = JSON_TYPE.OBJECT;
    foreach(k, v; a) {
      val.object[to!string(k)] = toJsonValue(v);
    }
  } else static if(isArray!(T)) {
    val.type = JSON_TYPE.ARRAY;
    val.array.length = a.length;
    foreach(i, v; a) {
      val.array[i] = toJsonValue(v);
    }
  } else static if(is(T == struct)) {
    val.type = JSON_TYPE.OBJECT;

    foreach(i, member; a.tupleof) {
      string name = a.tupleof[i].stringof[2..$];
      static if(a.tupleof[i].stringof[2] != '_')
        val.object[name] = toJsonValue(member);
    }
  } else { /* our catch all is to just do strings */
    val.type = JSON_TYPE.STRING         ;
    val.str = to!string(a);
  }

  return val;
}
