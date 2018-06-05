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
	std::map<CORBA::UShort, ToneControl*>::iterator it;

	for (it = tone_map.begin(); it != tone_map.end(); ++it ) {
		it->second->stop();
		delete it->second;
	}
	tone_map.clear();
}

CORBA::UShort Audio_AudibleAlertsAndAlarms_In_i::createTone(const Audio::AudibleAlertsAndAlarms::ToneProfileType& toneProfile)
	throw (Audio::AudibleAlertsAndAlarms::InvalidToneProfile)
{
    boost::mutex::scoped_lock lock(portAccess);

    std::map<CORBA::UShort, ToneControl*>::iterator it;
    CORBA::UShort toneId = 0;

    if(toneProfile._d() == Audio::AudibleAlertsAndAlarms::SIMPLE_TONE){
    	const Audio::AudibleAlertsAndAlarms::SimpleToneProfile simple = toneProfile.simpleTone();

    	if(simple.frequencyInHz > 4000 && simple.frequencyInHz < 50){
    		throw Audio::AudibleAlertsAndAlarms::InvalidToneProfile(true, false, "frequencyInHz out of range [50,4000]");
    	}

    	if(!simple.durationPerBurstInMs){
    		throw Audio::AudibleAlertsAndAlarms::InvalidToneProfile(true, false, "durationPerBurstInMs out of range [1,65535]");
    	}

    	if(simple.repeatIntervalInMs && simple.repeatIntervalInMs < simple.durationPerBurstInMs){
    		throw Audio::AudibleAlertsAndAlarms::InvalidToneProfile(true, false, "repeatIntervalInMs must be 0 or greater than durationPerBurstInMs");
    	}

    }else{
    	const Audio::AudibleAlertsAndAlarms::ComplexToneProfile complex = toneProfile.complexTone();
    	int sample_counter = 0;

    	for (it=tone_map.begin(); it!=tone_map.end(); ++it){
    		sample_counter += it->second->getNumSamples();
    	}

    	if((sample_counter + complex.toneSamples.length()) > 196608){
    		throw Audio::AudibleAlertsAndAlarms::InvalidToneProfile(false, true, "Cannot store more than 196608 samples");
    	}
    }

    while((it = tone_map.find(toneId)) != tone_map.end()){
    	toneId++;
    }

    tone_map.insert(std::make_pair(toneId, new ToneControl(toneProfile, parent)));

    return toneId;
}

void Audio_AudibleAlertsAndAlarms_In_i::startTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    std::map<CORBA::UShort, ToneControl*>::iterator it;

    it = tone_map.find(toneId);
    if(it == tone_map.end()){
    	throw Audio::AudibleAlertsAndAlarms::InvalidToneId("Invalid ToneId");
    }

	it->second->start();
}

void Audio_AudibleAlertsAndAlarms_In_i::stopTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    std::map<CORBA::UShort, ToneControl*>::iterator it;

	it = tone_map.find(toneId);
	if(it == tone_map.end()){
		throw Audio::AudibleAlertsAndAlarms::InvalidToneId("Invalid ToneId");
	}

	it->second->stop();
}

void Audio_AudibleAlertsAndAlarms_In_i::destroyTone(CORBA::UShort toneId) throw (Audio::AudibleAlertsAndAlarms::InvalidToneId)
{
    boost::mutex::scoped_lock lock(portAccess);
    std::map<CORBA::UShort, ToneControl*>::iterator it;

	it = tone_map.find(toneId);
	if(it == tone_map.end()){
		throw Audio::AudibleAlertsAndAlarms::InvalidToneId("Invalid ToneId");
	}

	it->second->stop();
	delete it->second;

	tone_map.erase(it);
}

void Audio_AudibleAlertsAndAlarms_In_i::stopAllTones()
{
    boost::mutex::scoped_lock lock(portAccess);
    std::map<CORBA::UShort, ToneControl*>::iterator it;
	for (it = tone_map.begin(); it != tone_map.end(); ++it ) {
		it->second->stop();
	}
}

std::string Audio_AudibleAlertsAndAlarms_In_i::getRepid() const
{
    return Audio::AudibleAlertsAndAlarms::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Audio_SampleStreamControl_In_i definition
// ----------------------------------------------------------------------------------------
Audio_SampleStreamControl_In_i::Audio_SampleStreamControl_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

Audio_SampleStreamControl_In_i::~Audio_SampleStreamControl_In_i()
{
}

void Audio_SampleStreamControl_In_i::setMaxPayloadSize(CORBA::ULong maxPayloadSize) throw (JTRS::InvalidParameter)
{
    boost::mutex::scoped_lock lock(portAccess);

    if(pthread_mutex_trylock(&parent->tx_stream_lock) == 0){
    	if(maxPayloadSize <= MAX_PAYLOAD_SIZE_H && maxPayloadSize >= 1){
			parent->tx_payload_size = maxPayloadSize;
			if(parent->tx_override_timeout < (maxPayloadSize*1000)/parent->capture_sample_rate){
				parent->tx_override_timeout = (maxPayloadSize*1000)/parent->capture_sample_rate + 5;
			}
		}else{
			throw JTRS::InvalidParameter();
		}
    	pthread_mutex_unlock(&parent->tx_stream_lock);
    }

}

void Audio_SampleStreamControl_In_i::setMinPayloadSize(CORBA::ULong minPayloadSize) throw (JTRS::InvalidParameter)
{
    boost::mutex::scoped_lock lock(portAccess);

    if(pthread_mutex_trylock(&parent->tx_stream_lock) == 0){
    	if(minPayloadSize <= MIN_PAYLOAD_SIZE_H && minPayloadSize >= 0 && minPayloadSize <= parent->tx_payload_size){
			// Nothing to be done
		}else{
			throw JTRS::InvalidParameter();
		}
		pthread_mutex_unlock(&parent->tx_stream_lock);
	}

}

void Audio_SampleStreamControl_In_i::setDesiredPayloadSize(CORBA::ULong desiredPayloadSize) throw (JTRS::InvalidParameter)
{
    boost::mutex::scoped_lock lock(portAccess);

    if(pthread_mutex_trylock(&parent->tx_stream_lock) == 0){
    	if(desiredPayloadSize <= MAX_PAYLOAD_SIZE_H && desiredPayloadSize >= 1){
			parent->tx_payload_size = desiredPayloadSize;
			if(parent->tx_override_timeout < (desiredPayloadSize*1000)/parent->capture_sample_rate){
				parent->tx_override_timeout = (desiredPayloadSize*1000)/parent->capture_sample_rate + 5;
			}
		}else{
			throw JTRS::InvalidParameter();
		}
		pthread_mutex_unlock(&parent->tx_stream_lock);
	}

}

void Audio_SampleStreamControl_In_i::setMinOverrideTimeout(CORBA::ULong minOverrideTimeout) throw (JTRS::InvalidParameter)
{
    boost::mutex::scoped_lock lock(portAccess);

    if(pthread_mutex_trylock(&parent->tx_stream_lock) == 0){
    	if(minOverrideTimeout <= MIN_OVERRIDE_TIMEOUT_H
    			&& minOverrideTimeout >= (parent->tx_payload_size*1000)/parent->capture_sample_rate){
			parent->tx_override_timeout = minOverrideTimeout;
		}else{
			throw JTRS::InvalidParameter();
		}
		pthread_mutex_unlock(&parent->tx_stream_lock);
	}
}

std::string Audio_SampleStreamControl_In_i::getRepid() const
{
    return Packet::PayloadControl::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Audio_SampleStream_In_i definition
// ----------------------------------------------------------------------------------------
Audio_SampleStream_In_i::Audio_SampleStream_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);

    rx_min_override_timeout = 23;
	rx_max_payload_size = 320;
	rx_min_payload_size = 320;
	rx_desired_payload_size = 320;
}

Audio_SampleStream_In_i::~Audio_SampleStream_In_i()
{
	std::map<Packet::Stream, StreamControl>::iterator it;
	for (it = stream_map.begin(); it != stream_map.end(); ++it) {
		snd_pcm_drop(it->second.pcm_handle);
		snd_pcm_close(it->second.pcm_handle);
	}
	stream_map.clear();
}

CORBA::ULong Audio_SampleStream_In_i::getMaxPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    // This should be tuned according to the hardware
    return rx_max_payload_size;
}

CORBA::ULong Audio_SampleStream_In_i::getMinPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    // This should be tuned according to the hardware
    return rx_min_payload_size;
}

CORBA::ULong Audio_SampleStream_In_i::getDesiredPayloadSize()
{
    boost::mutex::scoped_lock lock(portAccess);
    // This should be tuned according to the hardware
    return rx_desired_payload_size;
}

CORBA::ULong Audio_SampleStream_In_i::getMinOverrideTimeout()
{
    boost::mutex::scoped_lock lock(portAccess);
    // This should be tuned according to the hardware
    return rx_min_override_timeout;
}

void Audio_SampleStream_In_i::pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload)
{
	int err;
    boost::mutex::scoped_lock lock(portAccess);

    std::map<Packet::Stream, StreamControl>::iterator it;

    it = stream_map.find(control.streamId);
    if(it == stream_map.end()){
    	// create new stream if new streamId is received
    	std::pair<Packet::Stream, StreamControl> new_stream;
    	new_stream.first = control.streamId;
    	new_stream.second.seq_number = control.sequenceNumber; // should be zero

    	if(stream_map.empty()){
			pthread_mutex_lock(&parent->rx_lock);
			parent->rx_active = true;
			pthread_mutex_unlock(&parent->rx_lock);
		}

    	if(!(rx_desired_payload_size = AudioPortDevice_i::init_pcm(
    			&new_stream.second.pcm_handle,
				parent->output_device_name.c_str(),
				SND_PCM_STREAM_PLAYBACK,
				&parent->playback_sample_rate,
				SND_PCM_FORMAT_U16_LE,
				0))){
    		throw CF::Device::InvalidState("Cannot initialize output audio device!");
    	}else{
    		rx_max_payload_size = 4*rx_desired_payload_size;
    		rx_min_payload_size = rx_desired_payload_size;
    		rx_min_override_timeout = ((rx_desired_payload_size*1000)/parent->playback_sample_rate)+5;
    	}

    	/* Payload size should not be set while streaming
    	if(parent->audio_sample_stream_ctrl_uses_port->isActive()){
    		parent->audio_sample_stream_ctrl_uses_port->setDesiredPayloadSize(rx_desired_payload_size);
    		parent->audio_sample_stream_ctrl_uses_port->setMaxPayloadSize(rx_max_payload_size);
    		parent->audio_sample_stream_ctrl_uses_port->setMinPayloadSize(rx_desired_payload_size);
    		parent->audio_sample_stream_ctrl_uses_port->setMinOverrideTimeout(rx_min_override_timeout);
    	}*/

    	it = stream_map.insert(it, new_stream);

		if ((err = snd_pcm_prepare(it->second.pcm_handle)) < 0) {
			LOG_ERROR(AudioPortDevice_i, "cannot prepare audio interface for use ("<< snd_strerror(err)<<")");
			return;
		}

    }

    AudioPortDevice_i::writeBuffer(it->second.pcm_handle, payload.get_buffer(), payload.length(), sizeof(CORBA::UShort));

    if(it->second.seq_number != control.sequenceNumber){
		LOG_WARN(AudioPortDevice_i, "Sequence Number doesn't match, a packet might be lost");
		it->second.seq_number = control.sequenceNumber;
	}

    it->second.seq_number++;

    if(control.endOfStream){
    	// close stream
    	snd_pcm_drain(it->second.pcm_handle);
    	snd_pcm_close(it->second.pcm_handle);
    	stream_map.erase(it);

    	if(stream_map.empty()){
    		pthread_mutex_lock(&parent->rx_lock);
			parent->rx_active = false;
			pthread_mutex_unlock(&parent->rx_lock);
    	}
    }

}

std::string Audio_SampleStream_In_i::getRepid() const
{
    return Packet::UshortStream::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Audio_SampleMessageControl_In_i definition
// ----------------------------------------------------------------------------------------
Audio_SampleMessageControl_In_i::Audio_SampleMessageControl_In_i(std::string port_name, AudioPortDevice_base *_parent) : 
Port_Provides_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
}

Audio_SampleMessageControl_In_i::~Audio_SampleMessageControl_In_i()
{
}

CORBA::Boolean Audio_SampleMessageControl_In_i::rxActive()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::Boolean retval;
	pthread_mutex_lock(&parent->rx_lock);
	retval = parent->rx_active;
	pthread_mutex_unlock(&parent->rx_lock);

	return retval;
}

CORBA::Boolean Audio_SampleMessageControl_In_i::txActive()
{
    boost::mutex::scoped_lock lock(portAccess);
    CORBA::Boolean retval;
    pthread_mutex_lock(&parent->tx_lock);
    retval = parent->tx_active;
	pthread_mutex_unlock(&parent->tx_lock);

    return retval;
}

void Audio_SampleMessageControl_In_i::abortTx(CORBA::UShort streamId)
{
    boost::mutex::scoped_lock lock(portAccess);

    pthread_mutex_lock(&parent->tx_lock);
    parent->tx_active = false;
    parent->tx_abort = true;
	pthread_mutex_unlock(&parent->tx_lock);
	pthread_join(parent->tx_thread, NULL);
	parent->tx_abort = false;
}

std::string Audio_SampleMessageControl_In_i::getRepid() const
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
// Audio_SampleStreamControl_Out_i definition
// ----------------------------------------------------------------------------------------
PREPARE_ALT_LOGGING(Audio_SampleStreamControl_Out_i,AudioPortDevice_i)
Audio_SampleStreamControl_Out_i::Audio_SampleStreamControl_Out_i(std::string port_name, AudioPortDevice_base *_parent) :
Port_Uses_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
    recConnectionsRefresh = false;
    recConnections.length(0);
}

Audio_SampleStreamControl_Out_i::~Audio_SampleStreamControl_Out_i()
{
}

void Audio_SampleStreamControl_Out_i::setMaxPayloadSize(CORBA::ULong maxPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMaxPayloadSize(maxPayloadSize);
            } catch(...) {
                LOG_ERROR(Audio_SampleStreamControl_Out_i,"Call to setMaxPayloadSize by Audio_SampleStreamControl_Out_i failed");
                throw;
            }
        }
    }

}

void Audio_SampleStreamControl_Out_i::setMinPayloadSize(CORBA::ULong minPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMinPayloadSize(minPayloadSize);
            } catch(...) {
                LOG_ERROR(Audio_SampleStreamControl_Out_i,"Call to setMinPayloadSize by Audio_SampleStreamControl_Out_i failed");
                throw;
            }
        }
    }

}

void Audio_SampleStreamControl_Out_i::setDesiredPayloadSize(CORBA::ULong desiredPayloadSize)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setDesiredPayloadSize(desiredPayloadSize);
            } catch(...) {
                LOG_ERROR(Audio_SampleStreamControl_Out_i,"Call to setDesiredPayloadSize by Audio_SampleStreamControl_Out_i failed");
                throw;
            }
        }
    }

}

void Audio_SampleStreamControl_Out_i::setMinOverrideTimeout(CORBA::ULong minOverrideTimeout)
{
    std::vector < std::pair < Packet::PayloadControl_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->setMinOverrideTimeout(minOverrideTimeout);
            } catch(...) {
                LOG_ERROR(Audio_SampleStreamControl_Out_i,"Call to setMinOverrideTimeout by Audio_SampleStreamControl_Out_i failed");
                throw;
            }
        }
    }

}

std::string Audio_SampleStreamControl_Out_i::getRepid() const
{
    return Packet::PayloadControl::_PD_repoId;
}
// ----------------------------------------------------------------------------------------
// Audio_SampleStream_Out_i definition
// ----------------------------------------------------------------------------------------
PREPARE_ALT_LOGGING(Audio_SampleStream_Out_i,AudioPortDevice_i)
Audio_SampleStream_Out_i::Audio_SampleStream_Out_i(std::string port_name, AudioPortDevice_base *_parent) :
Port_Uses_base_impl(port_name)
{
    parent = static_cast<AudioPortDevice_i *> (_parent);
    recConnectionsRefresh = false;
    recConnections.length(0);
}

Audio_SampleStream_Out_i::~Audio_SampleStream_Out_i()
{
}

CORBA::ULong Audio_SampleStream_Out_i::getMaxPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMaxPayloadSize();
            } catch(...) {
                LOG_ERROR(Audio_SampleStream_Out_i,"Call to getMaxPayloadSize by Audio_SampleStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Audio_SampleStream_Out_i::getMinPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMinPayloadSize();
            } catch(...) {
                LOG_ERROR(Audio_SampleStream_Out_i,"Call to getMinPayloadSize by Audio_SampleStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Audio_SampleStream_Out_i::getDesiredPayloadSize()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getDesiredPayloadSize();
            } catch(...) {
                LOG_ERROR(Audio_SampleStream_Out_i,"Call to getDesiredPayloadSize by Audio_SampleStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

CORBA::ULong Audio_SampleStream_Out_i::getMinOverrideTimeout()
{
    CORBA::ULong retval = 0;
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                retval = ((*i).first)->getMinOverrideTimeout();
            } catch(...) {
                LOG_ERROR(Audio_SampleStream_Out_i,"Call to getMinOverrideTimeout by Audio_SampleStream_Out_i failed");
                throw;
            }
        }
    }

    return retval;
}

void Audio_SampleStream_Out_i::pushPacket(const Packet::StreamControlType& control, const JTRS::UshortSequence& payload)
{
    std::vector < std::pair < Packet::UshortStream_var, std::string > >::iterator i;

    boost::mutex::scoped_lock lock(updatingPortsLock);   // don't want to process while command information is coming in

    if (active) {
        for (i = outConnections.begin(); i != outConnections.end(); ++i) {
            try {
                ((*i).first)->pushPacket(control, payload);
            } catch(...) {
                LOG_ERROR(Audio_SampleStream_Out_i,"Call to pushPacket by Audio_SampleStream_Out_i failed");
                throw;
            }
        }
    }

}

std::string Audio_SampleStream_Out_i::getRepid() const
{
    return Packet::UshortStream::_PD_repoId;
}

