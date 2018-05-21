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

        CORBA::UShort createTone(const Audio::AudibleAlertsAndAlarms::ToneProfileType& toneProfile);
        void startTone(CORBA::UShort toneId);
        void stopTone(CORBA::UShort toneId);
        void destroyTone(CORBA::UShort toneId);
        void stopAllTones();
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleStreamControl_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStreamControl_In_i : public POA_Audio::SampleStreamControl, public Port_Provides_base_impl
{
    public:
        Audio_SampleStreamControl_In_i(std::string port_name, AudioPortDevice_base *_parent);
        ~Audio_SampleStreamControl_In_i();

        void setMaxPayloadSize(CORBA::ULong maxPayloadSize);
        void setMinPayloadSize(CORBA::ULong minPayloadSize);
        void setDesiredPayloadSize(CORBA::ULong desiredPayloadSize);
        void setMinOverrideTimeout(CORBA::ULong minOverrideTimeout);
        std::string getRepid() const;

    protected:
        AudioPortDevice_i *parent;
        boost::mutex portAccess;
};
// ----------------------------------------------------------------------------------------
// Audio_SampleStream_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleStream_In_i : public POA_Audio::SampleStream, public Port_Provides_base_impl
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
};
// ----------------------------------------------------------------------------------------
// Audio_SampleMessageControl_In_i declaration
// ----------------------------------------------------------------------------------------
class Audio_SampleMessageControl_In_i : public POA_Audio::SampleMessageControl, public Port_Provides_base_impl
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
            Audio::SampleStreamControl_var port = Audio::SampleStreamControl::_narrow(connection);
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

        std::vector< std::pair<Audio::SampleStreamControl_var, std::string> > _getConnections()
        {
            return outConnections;
        }

    protected:
        AudioPortDevice_i *parent;
        std::vector < std::pair<Audio::SampleStreamControl_var, std::string> > outConnections;
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
            Audio::SampleStream_var port = Audio::SampleStream::_narrow(connection);
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

        std::vector< std::pair<Audio::SampleStream_var, std::string> > _getConnections()
        {
            return outConnections;
        }

    protected:
        AudioPortDevice_i *parent;
        std::vector < std::pair<Audio::SampleStream_var, std::string> > outConnections;
        ExtendedCF::UsesConnectionSequence recConnections;
        bool recConnectionsRefresh;
};
#endif // PORT_H
