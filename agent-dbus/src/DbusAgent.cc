/*
 * File:	DbusAgent.cc
 *
 * Authors:	Arvin Schnell <aschnell@suse.de>
 */

#include "config.h"

#include <YCP.h>
#include <ycp/y2log.h>

#include "DbusAgent.h"


DbusAgent::DbusAgent()
    : connection(NULL)
{
    dbus_error_init(&error);
}


DbusAgent::~DbusAgent()
{
    disconnect();
}

// FIXME YCPValue functions _copy-pasted_ from yast2-dbus-server! 
// (but only the simple versions, which are enough...)

//
// returns YCPNull without an error if "it" does not point to an integer/byte
YCPValue DbusAgent::getYCPValueInteger(DBusMessageIter *it) const
{
    int type = dbus_message_iter_get_arg_type(it);
    YCPValue ret;

    switch (type) {
	case DBUS_TYPE_INT64:
	{
	    dbus_int64_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_UINT64:
	{
	    // warning: YCPInteger is signed!
	    dbus_uint64_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_INT32:
	{
	    dbus_int32_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_UINT32:
	{
	    dbus_uint32_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_INT16:
	{
	    dbus_int16_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_UINT16:
	{
	    dbus_uint16_t i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
	case DBUS_TYPE_BYTE:
	{
	    unsigned char i;
	    dbus_message_iter_get_basic(it, &i);
	    ret = YCPInteger(i);
	    break;
	}
    }
    return ret;
}

// "it" is the inside iterator
YCPMap DbusAgent::getYCPValueMap(DBusMessageIter *it) const
{
    YCPMap map;

    while (dbus_message_iter_get_arg_type (it) != DBUS_TYPE_INVALID)
    {
        int type = dbus_message_iter_get_arg_type(it);

	// is it a map or a list?
	if (type == DBUS_TYPE_DICT_ENTRY)
	{
	    DBusMessageIter mapit;
	    dbus_message_iter_recurse(it, &mapit);

	    // DICT keys cannot be structs, skip Bsv
	    YCPValue key = getYCPValueRawAny (&mapit);

	    dbus_message_iter_next(&mapit);

	    // read the value
	    YCPValue val = getYCPValueRawAny (&mapit);

	    map->add(key, val);
	}
        else
        {
            y2error ("Does not look like a map: %c", type);
        }

	dbus_message_iter_next(it);
    }
    return map;
}

YCPValue DbusAgent::getYCPValueRawAny(DBusMessageIter *it) const
{
    YCPValue ret;

    int type = dbus_message_iter_get_arg_type(it);
    if (type == DBUS_TYPE_BOOLEAN)
    {
	dbus_bool_t b; // not bool, bnc#606712
	dbus_message_iter_get_basic(it, &b);
	ret = YCPBoolean(b);
    }
    else if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH)
    {
	const char *s;
	dbus_message_iter_get_basic(it, &s);
	ret = YCPString(s);
    }
    else if (type == DBUS_TYPE_ARRAY || type == DBUS_TYPE_STRUCT)
    {
	DBusMessageIter sub;
	dbus_message_iter_recurse(it, &sub);

	// DBUS_TYPE_ARRAY is used for YCPList and YCPMap
	y2debug ("Reading RAW DBus array");

	// is the container a map or a list? => check the signature of the iterator
	// hash signature starts with '{', examples: list signature: "s", map signature: "{sv}"
	std::string cont_sig(dbus_message_iter_get_signature(&sub));
	y2debug ("Container signature: %s", cont_sig.c_str());

	if (!cont_sig.empty() && cont_sig[0] == '{')
	{
	    y2debug ("Found a map");
	    ret = getYCPValueMap(&sub);
	}
	else
	{
	    y2debug ("Found a list or struct (%c)", type);

	    YCPList lst;

	    while (dbus_message_iter_get_arg_type (&sub) != DBUS_TYPE_INVALID)
	    {
		YCPValue list_val = getYCPValueRawAny (&sub);
		lst->add(list_val);

		dbus_message_iter_next(&sub);
	    }

	    ret = lst;
	}
    }
    else if (type == DBUS_TYPE_DOUBLE)
    {
	double d;
	dbus_message_iter_get_basic(it, &d);
	ret = YCPFloat(d);
    }
    else if (type == DBUS_TYPE_VARIANT)
    {
	DBusMessageIter sub;
	dbus_message_iter_recurse(it, &sub);

	y2debug ("Found a DBus variant");
	YCPValue val;

	// there should be just one value inside the container
	if (dbus_message_iter_get_arg_type (&sub) != DBUS_TYPE_INVALID)
	{
	    val = getYCPValueRawAny(&sub);
	}
	// FIXME YCPNull possible
	ret = val;
    }
    else
    {
	ret = getYCPValueInteger(it);
	if (ret.isNull())
	{
	    y2error ("Unsupported DBus type: %d (%c)", type, (char)type);
	}
    }

    return ret;
}

YCPValue
DbusAgent::Read(const YCPPath& path, const YCPValue& arg, const YCPValue&)
{
    y2debug("Read(%s)", path->toString().c_str());

    if (path->isRoot())
    {
	ycp2error("Read() called without sub-path");
	return YCPNull();
    }

    const string cmd = path->component_str(0); // just a shortcut

    return YCPError(string("Undefined subpath for Read(") + path->toString() + ")");
}


YCPBoolean
DbusAgent::Write(const YCPPath& path, const YCPValue& value,
		 const YCPValue& arg)
{
    y2debug("Write(%s)", path->toString().c_str());

    if (path->isRoot())
    {
	ycp2error("Write() called without sub-path");
	return YCPBoolean(false);
    }

    const string cmd = path->component_str(0); // just a shortcut

    ycp2error("Undefined subpath for Write(%s)", path->toString ().c_str ());
    return YCPBoolean(false);
}


YCPValue
DbusAgent::Execute(const YCPPath& path, const YCPValue& value,
		   const YCPValue& arg)
{
    y2debug("Execute(%s)", path->toString().c_str());

    if (path->isRoot())
    {
	return YCPError("Execute() called without sub-path");
    }

    if (value.isNull())
    {
	return YCPError(string("Execute(")+path->toString()+") without argument.");
    }

    const string cmd = path->component_str(0); // just a shortcut

    if (cmd == "method")
    {
	/**
	 * @builtin Execute(.dbus.method, map params, list args) -> boolean
	 *
	 * params must contain parameters for dbus_message_new_method_call()
	 * and args must contain arguments for dbus method call.
	 */

	if (!connection)
	    return YCPError("dbus connection failed");

	if (!value->isMap())
	    return YCPError("value not a map");

	if (!arg->isList())
	    return YCPError("arg not a list");

	YCPMap tmp1 = value->asMap();

	YCPValue destination = tmp1.value(YCPSymbol("destination"));
	if (!destination->isString())
	    return YCPError("Missing or wrong type for 'destination'");

	YCPValue path = tmp1.value(YCPSymbol("path"));
	if (!path->isString())
	    return YCPError("Missing or wrong type for 'path'");

	YCPValue interface = tmp1.value(YCPSymbol("interface"));
	if (!interface->isString())
	    return YCPError("Missing or wrong type for 'interface'");

	YCPValue method = tmp1.value(YCPSymbol("method"));
	if (!method->isString())
	    return YCPError("Missing or wrong type for 'method'");

	DBusMessage* message = dbus_message_new_method_call(destination->asString()->value_cstr(),
							    path->asString()->value_cstr(),
							    interface->asString()->value_cstr(),
							    method->asString()->value_cstr());
	if (NULL == message)
	{
	    return YCPError("dbus_message_new_method_call() failed");
	}

	DBusMessageIter args;

	dbus_message_iter_init_append(message, &args);

	YCPList tmp2 = arg->asList();
	for (YCPListIterator it = tmp2.begin(); it != tmp2.end(); ++it)
	{
	    const YCPValue& tmp3 = *it;

	    if (tmp3->isString())
	    {
		const char* param = tmp3->asString()->value_cstr();
		if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &param))
		{
		    dbus_message_unref(message);
		    return YCPError("dbus_message_iter_append_basic() failed");
		}
	    }
	    else if (tmp3->isBoolean())
	    {
		bool param = tmp3->asBoolean()->value();
		if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_BOOLEAN, &param))
		{
		    dbus_message_unref(message);
		    return YCPError("dbus_message_iter_append_basic() failed");
		}
	    }
	    else
	    {
		dbus_message_unref(message);
		return YCPError("Unsupported type");
	    }
	}

	DBusMessage* reply = dbus_connection_send_with_reply_and_block(connection, message, -1, &error);

	dbus_message_unref(message);

	if (dbus_error_is_set(&error))
	{
 	    y2error("dbus_connection_send_with_reply_and_block() failed (%s: %s)", error.name, error.message);
	    dbus_error_free(&error);
            // FIXME save the error type for later retrieval (esp. permission error)
	    return YCPError("dbus_connection_send_with_reply_and_block() failed");
	}

	if (reply == NULL)
	{
	    return YCPError("dbus_connection_send_with_reply_and_block() failed");
	}

  	DBusMessageIter reply_iter;
 	dbus_message_iter_init(reply, &reply_iter);
           
        YCPValue result = getYCPValueRawAny (&reply_iter);

	dbus_message_unref(reply);

	return result;
    }

    return YCPError(string("Undefined subpath for Execute(") + path->toString() + ")");
}


YCPValue
DbusAgent::otherCommand(const YCPTerm& term)
{
    string sym = term->name();

    if (sym == "Bus")
    {
	if (term->size() != 1 || !term->value(0)->isString())
	{
	    return YCPError("Bad number of arguments. Expecting Bus (\"type\")");
	}

	string bus = term->value(0)->asString()->value();
	if (bus == "system")
	    connect(DBUS_BUS_SYSTEM);
	else if (bus == "session")
	    connect(DBUS_BUS_SESSION);
	else
	    return YCPError("Unknown bus");

	return YCPBoolean(true);
    }

    return YCPNull();
}


void
DbusAgent::connect(DBusBusType type)
{
    disconnect();

    y2milestone("connecting dbus");

    connection = dbus_bus_get(type, &error);
    if (dbus_error_is_set(&error))
    {
	y2error("dbus_bus_get() failed (%s)", error.message);
	dbus_error_free(&error);
    }

    if (connection == NULL)
    {
	y2error("connecting dbus failed");
    }
}


void
DbusAgent::disconnect()
{
    if (connection)
    {
	dbus_connection_unref(connection);
	connection = NULL;
    }
}
