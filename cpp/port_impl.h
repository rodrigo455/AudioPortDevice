/*
 * Author: Rodrigo Rolim Mendes de Alencar <alencar.fmce@imbel.gov.br>
 *
 * Copyright 2018 IMBEL/FMCE.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef PORT_H
#define PORT_H

#include <boost/thread/locks.hpp>
#include <ossie/Port_impl.h>
#include <ossie/debug.h>
#include <AUDIO/Audio.h>
#include <AUDIO/AudioSampleStreamExt.h>
#include <vector>
#include <utility>
#include <ossie/CF/QueryablePort.h>

struct StreamControl;
struct ToneControl;

class AudioPortDevice_base;
class AudioPortDevice_i;

#define CORBA_MAX_TRANSFER_BYTES omniORB::giopMaxMsgSize()

// ----------------------------------------------------------------------------------------
// Audio_AudibleAlertsAndAlarms_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_AudibleAlertsAndAlarms_In_i : public POA_Audio::AudibleAlertsAndAlarms, public Port_Provides_base_impl
{

    public:
        Audio_AudibleAlertsAndAlarms_In_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_AudibleAlertsAndAlarms_In_i();

        CORBA::UShort createTone(const Audio::AudibleAlertsAndAlarms::ToneProfileType& toneProfile) throw (Audio::AudibleAlertsAndAlarms::InvalidToneProfile);
        void startTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId);
        void stopTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId);
        void destroyTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId);
        void stopAllTones();
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;

        std::map<CORBA::UShort, ToneControl*> tone_map;

};
// ----------------------------------------------------------------------------------------
// Audio_SampleStreamControl_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStreamControl_In_i : public POA_Packet::PayloadControl, public Port_Provides_base_impl
{
    public:
        Audio_SampleStreamControl_In_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleStreamControl_In_i();

        void setMaxPayloadSize(CORBA::ULong maxPayloadSize) throw (JTRS::InvalidParameter);
        void setMinPayloadSize(CORBA::ULong minPayloadSize) throw (JTRS::InvalidParameter);
        void setDesiredPayloadSize(CORBA::ULong desiredPayloadSize) throw (JTRS::InvalidParameter);
        void setMinOverrideTimeout(CORBA::ULong minOverrideTimeout) throw (JTRS::InvalidParameter);
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleStream_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStream_In_i : public POA_Packet::UshortStream, public Port_Provides_base_impl
{
    public:
        Audio_SampleStream_In_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleStream_In_i();

        CORBA::ULong getMaxPayloadSize();
        CORBA::ULong getMinPayloadSize();
        CORBA::ULong getDesiredPayloadSize();
        CORBA::ULong getMinOverrideTimeout();
        void pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload);
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;

        CORBA::ULong rx_min_override_timeout;
		CORBA::ULong rx_max_sample_payload_size;
		CORBA::ULong rx_min_sample_payload_size;
		CORBA::ULong rx_desired_sample_payload_size;

        std::map<Packet::Stream, StreamControl> stream_map;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleMessageControl_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleMessageControl_In_i : public POA_DevMsgCtl::DeviceMessageControl, public Port_Provides_base_impl
{
    public:
        Audio_SampleMessageControl_In_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleMessageControl_In_i();

        CORBA::Boolean rxActive();
        CORBA::Boolean txActive();
        void abortTx(CORBA::UShort streamId);
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;
};
// ----------------------------------------------------------------------------------------
// Audio_AudioPTT_Signal_Out_i declaration
// ----------------------------------------------------------------------------------------
class Audio_AudioPTT_Signal_Out_i : public Port_Uses_base_impl, public POA_ExtendedCF::QueryablePort
{
    ENABLE_LOGGING
    public:
        Audio_AudioPTT_Signal_Out_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_AudioPTT_Signal_Out_i();

        void setPTT(CORBA::Boolean PTT);

        ExtendedCF::UsesConnectionSequence * connections() 
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            if (recConnectionsRefresh) {
                recConnections.length(outConnections.size());
                for (unsigned int i = 0; i < outConnections.size(); i++) {
                    recConnections[i].connectionId = CORBA::string_dup(outConnections[i].second.c_str());
                    recConnections[i].port = CORBA::Object::_duplicate(outConnections[i].first);
                }
                recConnectionsRefresh = false;
            }
            // NOTE: You must delete the object that this function returns!
            return new ExtendedCF::UsesConnectionSequence(recConnections);
        }

        void connectPort(CORBA::Object_ptr connection, const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            Audio::AudioPTT_Signal_var port = Audio::AudioPTT_Signal::_narrow(connection);
            outConnections.push_back(std::make_pair(port, connectionId));
            active = true;
            recConnectionsRefresh = true;
        }

        void disconnectPort(const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            for (unsigned int i = 0; i < outConnections.size(); i++) {
                if (outConnections[i].second == connectionId) {
                    outConnections.erase(outConnections.begin() + i);
                    break;
                }
            }

            if (outConnections.size() == 0) {
                active = false;
            }
            recConnectionsRefresh = true;
        }

        std::string getRepid () const;

        std::vector< std::pair<Audio::AudioPTT_Signal_var, std::string> > _getConnections()
        {
            return outConnections;
        }

    protected:
        AudioPortDevice_i *parent;
        std::vector < std::pair<Audio::AudioPTT_Signal_var, std::string> > outConnections;
        ExtendedCF::UsesConnectionSequence recConnections;
        bool recConnectionsRefresh;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleStreamControl_Out_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStreamControl_Out_i : public Port_Uses_base_impl, public POA_ExtendedCF::QueryablePort
{
    ENABLE_LOGGING
    public:
        Audio_SampleStreamControl_Out_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleStreamControl_Out_i();

        void setMaxPayloadSize(CORBA::ULong maxPayloadSize);
        void setMinPayloadSize(CORBA::ULong minPayloadSize);
        void setDesiredPayloadSize(CORBA::ULong desiredPayloadSize);
        void setMinOverrideTimeout(CORBA::ULong minOverrideTimeout);

        ExtendedCF::UsesConnectionSequence * connections() 
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            if (recConnectionsRefresh) {
                recConnections.length(outConnections.size());
                for (unsigned int i = 0; i < outConnections.size(); i++) {
                    recConnections[i].connectionId = CORBA::string_dup(outConnections[i].second.c_str());
                    recConnections[i].port = CORBA::Object::_duplicate(outConnections[i].first);
                }
                recConnectionsRefresh = false;
            }
            // NOTE: You must delete the object that this function returns!
            return new ExtendedCF::UsesConnectionSequence(recConnections);
        }

        void connectPort(CORBA::Object_ptr connection, const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            Packet::PayloadControl_var port = Packet::PayloadControl::_narrow(connection);
            outConnections.push_back(std::make_pair(port, connectionId));
            active = true;
            recConnectionsRefresh = true;
        }

        void disconnectPort(const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            for (unsigned int i = 0; i < outConnections.size(); i++) {
                if (outConnections[i].second == connectionId) {
                    outConnections.erase(outConnections.begin() + i);
                    break;
                }
            }

            if (outConnections.size() == 0) {
                active = false;
            }
            recConnectionsRefresh = true;
        }

        std::string getRepid () const;

        std::vector< std::pair<Packet::PayloadControl_var, std::string> > _getConnections()
        {
            return outConnections;
        }

    protected:
        AudioPortDevice_i *parent;
        std::vector < std::pair<Packet::PayloadControl_var, std::string> > outConnections;
        ExtendedCF::UsesConnectionSequence recConnections;
        bool recConnectionsRefresh;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleStream_Out_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStream_Out_i : public Port_Uses_base_impl, public POA_ExtendedCF::QueryablePort
{
    ENABLE_LOGGING
    public:
        Audio_SampleStream_Out_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleStream_Out_i();

        CORBA::ULong getMaxPayloadSize();
        CORBA::ULong getMinPayloadSize();
        CORBA::ULong getDesiredPayloadSize();
        CORBA::ULong getMinOverrideTimeout();
        void pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload);

        ExtendedCF::UsesConnectionSequence * connections() 
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            if (recConnectionsRefresh) {
                recConnections.length(outConnections.size());
                for (unsigned int i = 0; i < outConnections.size(); i++) {
                    recConnections[i].connectionId = CORBA::string_dup(outConnections[i].second.c_str());
                    recConnections[i].port = CORBA::Object::_duplicate(outConnections[i].first);
                }
                recConnectionsRefresh = false;
            }
            // NOTE: You must delete the object that this function returns!
            return new ExtendedCF::UsesConnectionSequence(recConnections);
        }

        void connectPort(CORBA::Object_ptr connection, const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            Packet::UshortStream_var port = Packet::UshortStream::_narrow(connection);
            outConnections.push_back(std::make_pair(port, connectionId));
            active = true;
            recConnectionsRefresh = true;
        }

        void disconnectPort(const char* connectionId)
        {
            boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in
            for (unsigned int i = 0; i < outConnections.size(); i++) {
                if (outConnections[i].second == connectionId) {
                    outConnections.erase(outConnections.begin() + i);
                    break;
                }
            }

            if (outConnections.size() == 0) {
                active = false;
            }
            recConnectionsRefresh = true;
        }

        std::string getRepid () const;

        std::vector< std::pair<Packet::UshortStream_var, std::string> > _getConnections()
        {
            return outConnections;
        }

    protected:
        AudioPortDevice_i *parent;
        std::vector < std::pair<Packet::UshortStream_var, std::string> > outConnections;
        ExtendedCF::UsesConnectionSequence recConnections;
        bool recConnectionsRefresh;
};
#endif // PORT_H
