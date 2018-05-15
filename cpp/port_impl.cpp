/*******************************************************************************************

    AUTO-GENERATED CODE. DO NOT MODIFY

    Source: AudioPortDevice.spd.xml

*******************************************************************************************/

#include "AudioPortDevice.h"

// ----------------------------------------------------------------------------------------
// Audio_AudibleAlertsAndAlarms_In_i definition
// ----------------------------------------------------------------------------------------
Audio_AudibleAlertsAndAlarms_In_i::Audio_AudibleAlertsAndAlarms_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

Audio_AudibleAlertsAndAlarms_In_i::~Audio_AudibleAlertsAndAlarms_In_i()
{
}

CORBA::UShort Audio_AudibleAlertsAndAlarms_In_i::createTone(const Audio::AudibleAlertsAndAlarms::ToneProfileType& toneProfile)
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::UShort retval = 0;
    // TODO: Fill in this function
    return retval;
}

void Audio_AudibleAlertsAndAlarms_In_i::startTone(CORBA::UShort toneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Audio_AudibleAlertsAndAlarms_In_i::stopTone(CORBA::UShort toneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Audio_AudibleAlertsAndAlarms_In_i::destroyTone(CORBA::UShort toneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Audio_AudibleAlertsAndAlarms_In_i::stopAllTones()
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

std::string Audio_AudibleAlertsAndAlarms_In_i::getRepid() const
{
    return Audio::AudibleAlertsAndAlarms::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Packet_PayloadControl_In_i definition
// ----------------------------------------------------------------------------------------
Packet_PayloadControl_In_i::Packet_PayloadControl_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

Packet_PayloadControl_In_i::~Packet_PayloadControl_In_i()
{
}

void Packet_PayloadControl_In_i::setMaxPayloadSize(CORBA::ULong maxPayloadSize)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Packet_PayloadControl_In_i::setMinPayloadSize(CORBA::ULong minPayloadSize)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Packet_PayloadControl_In_i::setDesiredPayloadSize(CORBA::ULong desiredPayloadSize)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

void Packet_PayloadControl_In_i::setMinOverrideTimeout(CORBA::ULong minOverrideTimeout)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

std::string Packet_PayloadControl_In_i::getRepid() const
{
    return Packet::PayloadControl::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Packet_UshortStream_In_i definition
// ----------------------------------------------------------------------------------------
Packet_UshortStream_In_i::Packet_UshortStream_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

Packet_UshortStream_In_i::~Packet_UshortStream_In_i()
{
}

CORBA::ULong Packet_UshortStream_In_i::getMaxPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::ULong retval = 0;
    // TODO: Fill in this function
    return retval;
}

CORBA::ULong Packet_UshortStream_In_i::getMinPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::ULong retval = 0;
    // TODO: Fill in this function
    return retval;
}

CORBA::ULong Packet_UshortStream_In_i::getDesiredPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::ULong retval = 0;
    // TODO: Fill in this function
    return retval;
}

CORBA::ULong Packet_UshortStream_In_i::getMinOverrideTimeout()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::ULong retval = 0;
    // TODO: Fill in this function
    return retval;
}

void Packet_UshortStream_In_i::pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

std::string Packet_UshortStream_In_i::getRepid() const
{
    return Packet::UshortStream::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// DevMsgCtl_DeviceMessageControl_In_i definition
// ----------------------------------------------------------------------------------------
DevMsgCtl_DeviceMessageControl_In_i::DevMsgCtl_DeviceMessageControl_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

DevMsgCtl_DeviceMessageControl_In_i::~DevMsgCtl_DeviceMessageControl_In_i()
{
}

CORBA::Boolean DevMsgCtl_DeviceMessageControl_In_i::rxActive()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::Boolean retval = 0;
    // TODO: Fill in this function
    return retval;
}

CORBA::Boolean DevMsgCtl_DeviceMessageControl_In_i::txActive()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::Boolean retval = 0;
    // TODO: Fill in this function
    return retval;
}

void DevMsgCtl_DeviceMessageControl_In_i::abortTx(CORBA::UShort streamId)
{
    boost::mutex::scoped_lock lock(portAccess);
    // TODO: Fill in this function
}

std::string DevMsgCtl_DeviceMessageControl_In_i::getRepid() const
{
    return DevMsgCtl::DeviceMessageControl::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Audio_AudioPTT_Signal_Out_i definition
// ----------------------------------------------------------------------------------------
PREPARE_ALT_LOGGING(Audio_AudioPTT_Signal_Out_i,AudioPortDevice_i)
Audio_AudioPTT_Signal_Out_i::Audio_AudioPTT_Signal_Out_i(std::string port_name, AudioPortDevice_base *_parent) :
Port_Uses_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
    recConnectionsRefresh = false;
    recConnections.length(0);
}

Audio_AudioPTT_Signal_Out_i::~Audio_AudioPTT_Signal_Out_i()
{
}

void Audio_AudioPTT_Signal_Out_i::setPTT(CORBA::Boolean PTT)
{
    std::vector < std::pair < Audio::AudioPTT_Signal_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setPTT(PTT);
            } catch(...) {
                LOG_ERROR(Audio_AudioPTT_Signal_Out_i,"Call to setPTT by Audio_AudioPTT_Signal_Out_i failed");
                throw;
            }
        }
    }

}

std::string Audio_AudioPTT_Signal_Out_i::getRepid() const
{
    return Audio::AudioPTT_Signal::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Packet_PayloadControl_Out_i definition
// ----------------------------------------------------------------------------------------
PREPARE_ALT_LOGGING(Packet_PayloadControl_Out_i,AudioPortDevice_i)
Packet_PayloadControl_Out_i::Packet_PayloadControl_Out_i(std::string port_name, AudioPortDevice_base *_parent) :
Port_Uses_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
    recConnectionsRefresh = false;
    recConnections.length(0);
}

Packet_PayloadControl_Out_i::~Packet_PayloadControl_Out_i()
{
}

void Packet_PayloadControl_Out_i::setMaxPayloadSize(CORBA::ULong maxPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMaxPayloadSize(maxPayloadSize);
            } catch(...) {
                LOG_ERROR(Packet_PayloadControl_Out_i,"Call to setMaxPayloadSize by Packet_PayloadControl_Out_i failed");
                throw;
            }
        }
    }

}

void Packet_PayloadControl_Out_i::setMinPayloadSize(CORBA::ULong minPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMinPayloadSize(minPayloadSize);
            } catch(...) {
                LOG_ERROR(Packet_PayloadControl_Out_i,"Call to setMinPayloadSize by Packet_PayloadControl_Out_i failed");
                throw;
            }
        }
    }

}

void Packet_PayloadControl_Out_i::setDesiredPayloadSize(CORBA::ULong desiredPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setDesiredPayloadSize(desiredPayloadSize);
            } catch(...) {
                LOG_ERROR(Packet_PayloadControl_Out_i,"Call to setDesiredPayloadSize by Packet_PayloadControl_Out_i failed");
                throw;
            }
        }
    }

}

void Packet_PayloadControl_Out_i::setMinOverrideTimeout(CORBA::ULong minOverrideTimeout)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMinOverrideTimeout(minOverrideTimeout);
            } catch(...) {
                LOG_ERROR(Packet_PayloadControl_Out_i,"Call to setMinOverrideTimeout by Packet_PayloadControl_Out_i failed");
                throw;
            }
        }
    }

}

std::string Packet_PayloadControl_Out_i::getRepid() const
{
    return Packet::PayloadControl::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Packet_UshortStream_Out_i definition
// ----------------------------------------------------------------------------------------
PREPARE_ALT_LOGGING(Packet_UshortStream_Out_i,AudioPortDevice_i)
Packet_UshortStream_Out_i::Packet_UshortStream_Out_i(std::string port_name, AudioPortDevice_base *_parent) :
Port_Uses_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
    recConnectionsRefresh = false;
    recConnections.length(0);
}

Packet_UshortStream_Out_i::~Packet_UshortStream_Out_i()
{
}

CORBA::ULong Packet_UshortStream_Out_i::getMaxPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMaxPayloadSize();
            } catch(...) {
                LOG_ERROR(Packet_UshortStream_Out_i,"Call to getMaxPayloadSize by Packet_UshortStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Packet_UshortStream_Out_i::getMinPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMinPayloadSize();
            } catch(...) {
                LOG_ERROR(Packet_UshortStream_Out_i,"Call to getMinPayloadSize by Packet_UshortStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Packet_UshortStream_Out_i::getDesiredPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getDesiredPayloadSize();
            } catch(...) {
                LOG_ERROR(Packet_UshortStream_Out_i,"Call to getDesiredPayloadSize by Packet_UshortStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Packet_UshortStream_Out_i::getMinOverrideTimeout()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMinOverrideTimeout();
            } catch(...) {
                LOG_ERROR(Packet_UshortStream_Out_i,"Call to getMinOverrideTimeout by Packet_UshortStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

void Packet_UshortStream_Out_i::pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload)
{
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->pushPacket(control, payload);
            } catch(...) {
                LOG_ERROR(Packet_UshortStream_Out_i,"Call to pushPacket by Packet_UshortStream_Out_i failed");
                throw;
            }
        }
    }

}

std::string Packet_UshortStream_Out_i::getRepid() const
{
    return Packet::UshortStream::_PD_repoId;
}

