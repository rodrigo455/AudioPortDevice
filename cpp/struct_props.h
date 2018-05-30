#ifndef STRUCTPROPS_H
#define STRUCTPROPS_H

/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

*******************************************************************************************/

#include <ossie/CorbaUtils.h>
#include <CF/cf.h>
#include <ossie/PropertyMap.h>

struct sample_stream_in_pktcfg_struct {
    sample_stream_in_pktcfg_struct ()
    {
        max_payload_size = 320;
        min_payload_size = 320;
        desired_payload_size = 320;
        min_override_timeout = 25;
    }

    static std::string getId() {
        return std::string("sample_stream_in_pktcfg");
    }

    static const char* getFormat() {
        return "IIII";
    }

    CORBA::ULong max_payload_size;
    CORBA::ULong min_payload_size;
    CORBA::ULong desired_payload_size;
    CORBA::ULong min_override_timeout;
};

inline bool operator>>= (const CORBA::Any& a, sample_stream_in_pktcfg_struct& s) {
    CF::Properties* temp;
    if (!(a >>= temp)) return false;
    const redhawk::PropertyMap& props = redhawk::PropertyMap::cast(*temp);
    if (props.contains("max_payload_size")) {
        if (!(props["max_payload_size"] >>= s.max_payload_size)) return false;
    }
    if (props.contains("min_payload_size")) {
        if (!(props["min_payload_size"] >>= s.min_payload_size)) return false;
    }
    if (props.contains("desired_payload_size")) {
        if (!(props["desired_payload_size"] >>= s.desired_payload_size)) return false;
    }
    if (props.contains("min_override_timeout")) {
        if (!(props["min_override_timeout"] >>= s.min_override_timeout)) return false;
    }
    return true;
}

inline void operator<<= (CORBA::Any& a, const sample_stream_in_pktcfg_struct& s) {
    redhawk::PropertyMap props;
 
    props["max_payload_size"] = s.max_payload_size;
 
    props["min_payload_size"] = s.min_payload_size;
 
    props["desired_payload_size"] = s.desired_payload_size;
 
    props["min_override_timeout"] = s.min_override_timeout;
    a <<= props;
}

inline bool operator== (const sample_stream_in_pktcfg_struct& s1, const sample_stream_in_pktcfg_struct& s2) {
    if (s1.max_payload_size!=s2.max_payload_size)
        return false;
    if (s1.min_payload_size!=s2.min_payload_size)
        return false;
    if (s1.desired_payload_size!=s2.desired_payload_size)
        return false;
    if (s1.min_override_timeout!=s2.min_override_timeout)
        return false;
    return true;
}

inline bool operator!= (const sample_stream_in_pktcfg_struct& s1, const sample_stream_in_pktcfg_struct& s2) {
    return !(s1==s2);
}

#endif // STRUCTPROPS_H
