/**************************************************************************

    This is the device code. This file contains the child class where
    custom functionality can be added to the device. Custom
    functionality to the base class can be extended here. Access to
    the ports can also be done from this class

**************************************************************************/

#include "AudioPortDevice.h"

PREPARE_LOGGING(AudioPortDevice_i)

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl)
{
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
}

AudioPortDevice_i::~AudioPortDevice_i()
{
}

void AudioPortDevice_i::constructor()
{
    /***********************************************************************************
     This is the RH constructor. All properties are properly initialized before this function is called 
    ***********************************************************************************/

	int err, dir, nperiods;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_U16_LE;

	tx_active = false;
	tx_abort = false;
	rx_active = false;

	tx_stream = 0;

	pthread_mutex_init(&tx_lock, NULL);

	/* INPUT AUDIO DEVICE */

	if ((err = snd_pcm_open(&input_handle, input_device_name.c_str(), SND_PCM_STREAM_CAPTURE, 0)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot open audio device "<<input_device_name << " ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot open input audio device!");
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot allocate hardware parameter structure ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot allocate hardware parameter structure for input audio device!");
	}

	if ((err = snd_pcm_hw_params_any(input_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot initialize hardware parameter structure ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot initialize hardware parameter structure for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_access(input_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set access type ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set access type for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_format(input_handle, hw_params, format)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample format ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set sample format for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_rate_near(input_handle, hw_params, &sample_rate, 0)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample rate ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set sample rate for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_channels(input_handle, hw_params, 1)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set channel count ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set channel count for input audio device!");
	}

	if ((err = snd_pcm_hw_params(input_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set parameters ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set hardware parameters for input audio device!");
	}

	unsigned int min_nperiods, max_nperiods;
	snd_pcm_hw_params_get_periods_min(hw_params, &min_nperiods, &dir);
	snd_pcm_hw_params_get_periods_max(hw_params, &max_nperiods, &dir);

	unsigned int orig_nperiods = 4;
	nperiods = std::min(std::max(min_nperiods, (unsigned int)4), max_nperiods);

	unsigned int period_time_us = (0.01 * 1e6 * orig_nperiods) / nperiods;

	dir = 0;
	if ((err = snd_pcm_hw_params_set_period_time_near(input_handle, hw_params, &period_time_us, &dir)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "set_period_time_near failed ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set period_time_near for input audio device!");
	}

	dir = 0;
	if ((err = snd_pcm_hw_params_get_period_size(hw_params, &input_period_size, &dir)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "get_period_size failed ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot get period size for input audio device!");
	}

	snd_pcm_hw_params_free(hw_params);

	input_buffer = (char*)malloc(input_period_size * 1 * snd_pcm_format_size(format, 1)); /* times number of hw channels*/

	/* PTT INPUT EVENT DEVICE*/

	ptt_fd = open(ptt_device.c_str(), O_RDONLY);
	if(ptt_fd < 0){
		LOG_ERROR(AudioPortDevice_i, "ptt_device ("<<ptt_device<<") could not be opened " << ptt_fd);
		throw CF::Device::InvalidState("Cannot open ptt input event device!");
	}

}

void AudioPortDevice_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	AudioPortDevice_base::start();

	pthread_create(&ptt_thread, NULL, &AudioPortDevice_i::ptt_thread_helper, this);


}

void AudioPortDevice_i::stop() throw (CORBA::SystemException, CF::Resource::StopError) {

	pthread_cancel(ptt_thread);
	pthread_join(ptt_thread, NULL);

	pthread_mutex_destroy(&tx_lock);

	LOG_INFO(AudioPortDevice_i, "ptt thread stopped");

	AudioPortDevice_base::stop();
}

void AudioPortDevice_i::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{

	free(input_buffer);
	snd_pcm_close(input_handle);

	close(ptt_fd);

	AudioPortDevice_base::releaseObject();
}


/**************************************************************************

    This is called automatically after allocateCapacity or deallocateCapacity are called.
    Your implementation should determine the current state of the device:

       setUsageState(CF::Device::IDLE);   // not in use
       setUsageState(CF::Device::ACTIVE); // in use, with capacity remaining for allocation
       setUsageState(CF::Device::BUSY);   // in use, with no capacity remaining for allocation

**************************************************************************/
void AudioPortDevice_i::updateUsageState()
{
}

int AudioPortDevice_i::serviceFunction()
{
    return FINISH;
}

void AudioPortDevice_i::txThread(){
	LOG_INFO(AudioPortDevice_i, "inputThread running period_size: " << input_period_size);

	CORBA::UShort *buf = (CORBA::UShort *)input_buffer;
	unsigned int sizeof_frame = 1 * sizeof(CORBA::UShort);
	int err;
	Packet::SeqNum sequenceNumber = 0;

	if ((err = snd_pcm_prepare (input_handle)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot prepare audio interface for use ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot prepare input audio device interface for use!");
	}

	/* BULKIO TEST TODO remove this
	BULKIO::StreamSRI sri = BULKIO::StreamSRI();
	sri.xdelta = 1.0/sample_rate;
	sri.xunits = BULKIO::UNITS_TIME;
	sri.streamID = "test";
	test_input->pushSRI(sri);*/


	pthread_mutex_lock(&tx_lock);

	while(tx_active){

		pthread_mutex_unlock(&tx_lock);

		if(!readBuffer(buf, input_period_size, sizeof_frame)){
			LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
			throw CF::Device::InvalidState("input buffer cannot be read!");
		}

		/* BULKIO TEST TODO remove this
		if(test_input->isActive()){
			test_input->pushPacket(buf, input_period_size, bulkio::time::utils::now(), false, "test");
		}*/

		if(audio_sample_stream_uses_port->isActive()){
			audio_sample_stream_uses_port->pushPacket(
					(Packet::StreamControlType){false, tx_stream, sequenceNumber, false},
					JTRS::UshortSequence(input_period_size,input_period_size,buf,0));
		}

		sequenceNumber++;

		pthread_mutex_lock(&tx_lock);
	}

	pthread_mutex_unlock(&tx_lock);

	// repeat one more time

	if(!readBuffer(buf, input_period_size, sizeof_frame)){
		LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
		throw CF::Device::InvalidState("input buffer cannot be read!");
	}

	/* BULKIO TEST TODO remove this
	if(test_input->isActive()){
		test_input->pushPacket(buf, input_period_size, bulkio::time::utils::now(), false, "test");
	}*/

	if(audio_sample_stream_uses_port->isActive()){
		audio_sample_stream_uses_port->pushPacket(
				(Packet::StreamControlType){true, tx_stream, sequenceNumber, tx_abort},
				JTRS::UshortSequence(input_period_size, tx_abort? 0:input_period_size, buf, 0));
	}

	snd_pcm_drop(input_handle);

	tx_stream++;
}

bool AudioPortDevice_i::readBuffer(void *vbuffer, unsigned nframes, unsigned sizeof_frame){

	unsigned char *buffer = (unsigned char*) vbuffer;

	while (nframes > 0) {
		int err = snd_pcm_readi(input_handle, buffer, nframes);
		if (err == -EAGAIN)
			continue;   // try again

		else if (err == -EPIPE) {  // overrun
			fputs("aO", stderr);
			if ((err = snd_pcm_prepare(input_handle)) < 0) {
				LOG_ERROR(AudioPortDevice_i, "snd_pcm_prepare failed. Can't recover from overrun");
				return false;
			}
			continue;  // try again
		} else if (err < 0) {
			LOG_ERROR(AudioPortDevice_i, "snd_pcm_readi failed");
			return false;
		}

		nframes -= err;
		buffer += err * sizeof_frame;
	}

	return true;
}

void AudioPortDevice_i::pttThread()
{
	struct input_event event;

	while(1){

		read(ptt_fd, &event, sizeof(struct input_event));
		/* Using left Ctrl key as PTT */
		if(event.type == EV_KEY && event.code == KEY_LEFTCTRL){

			if(event.value == EV_RELEASED){
				LOG_DEBUG(AudioPortDevice_i, "PTT Released");
				pthread_mutex_lock(&tx_lock);
				tx_active = false;
				pthread_mutex_unlock(&tx_lock);
				pthread_join(tx_thread, NULL);

				if(audio_ptt_uses_port->isActive()){
					audio_ptt_uses_port->setPTT(false);
				}

			}else if(event.value == EV_PRESSED){
				LOG_DEBUG(AudioPortDevice_i, "PTT Pressed");
				pthread_mutex_lock(&tx_lock);
				tx_active = true;
				pthread_mutex_unlock(&tx_lock);
				pthread_create(&tx_thread, NULL, &AudioPortDevice_i::tx_thread_helper, this);

				if(audio_ptt_uses_port->isActive()){
					audio_ptt_uses_port->setPTT(true);
				}
			}
		}
	}
}

