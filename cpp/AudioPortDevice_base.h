#ifndef AUDIOPORTDEVICE_BASE_IMPL_BASE_H
#define AUDIOPORTDEVICE_BASE_IMPL_BASE_H

#include <boost/thread.hpp>
#include <ossie/Device_impl.h>
#include <ossie/ThreadedComponent.h>

#include "port_impl.h"

class AudioPortDevice_base : public Device_impl, protected ThreadedComponent
{
    friend class Audio_AudibleAlertsAndAlarms_In_i;
    friend class Audio_SampleStreamControl_In_i;
    friend class Audio_SampleStream_In_i;
    friend class Audio_SampleMessageControl_In_i;
    friend class Audio_AudioPTT_Signal_Out_i;
    friend class Audio_SampleStreamControl_Out_i;
    friend class Audio_SampleStream_Out_i;

    public:
        AudioPortDevice_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        AudioPortDevice_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        AudioPortDevice_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        AudioPortDevice_base(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~AudioPortDevice_base();

        void start() throw (CF::Resource::StartError, CORBA::SystemException);

        void stop() throw (CF::Resource::StopError, CORBA::SystemException);

        void releaseObject() throw (CF::LifeCycle::ReleaseError, CORBA::SystemException);

        void loadProperties();

    protected:
        // Member variables exposed as properties
        /// Property: device_kind
        std::string device_kind;
        /// Property: device_model
        std::string device_model;
        /// Property: ptt_device
        std::string ptt_device;
        /// Property: sample_rate
        CORBA::ULong sample_rate;
        /// Property: capture_card
        std::string capture_card;
        /// Property: capture_volume
        CORBA::ULong capture_volume;
        /// Property: capture_mixer_control
        std::string capture_mixer_control;
        /// Property: playback_card
        std::string playback_card;
        /// Property: playback_volume
        CORBA::ULong playback_volume;
        /// Property: playback_mixer_control
        std::string playback_mixer_control;

        // Ports
        /// Port: audio_alertalarm_wf_provides_port
        Audio_AudibleAlertsAndAlarms_In_i *audio_alertalarm_wf_provides_port;
        /// Port: audio_sample_stream_ctrl_provides_port
        Audio_SampleStreamControl_In_i *audio_sample_stream_ctrl_provides_port;
        /// Port: audio_sample_stream_provides_port
        Audio_SampleStream_In_i *audio_sample_stream_provides_port;
        /// Port: audio_sample_msg_ctrl_provides_port
        Audio_SampleMessageControl_In_i *audio_sample_msg_ctrl_provides_port;
        /// Port: audio_ptt_uses_port
        Audio_AudioPTT_Signal_Out_i *audio_ptt_uses_port;
        /// Port: audio_sample_stream_ctrl_uses_port
        Audio_SampleStreamControl_Out_i *audio_sample_stream_ctrl_uses_port;
        /// Port: audio_sample_stream_uses_port
        Audio_SampleStream_Out_i *audio_sample_stream_uses_port;

    private:
        void construct();
};
#endif // AUDIOPORTDEVICE_BASE_IMPL_BASE_H
