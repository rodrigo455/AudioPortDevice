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

	int err;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_format_t format = SND_PCM_FORMAT_U16_LE;

	// To get card names run the command:
	// aplay -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
	// arecord -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
	input_device_name = "plughw";
	output_device_name = "plug:dmix";

	tx_active = false;
	tx_abort = false;
	rx_active = false;

	tx_stream = 0;

	tx_desired_payload = 320;
	tx_override_timeout = 23;

	pthread_mutex_init(&tx_lock, NULL);
	pthread_mutex_init(&tx_stream_lock, NULL);

	pthread_mutex_init(&rx_lock, NULL);

	/* INPUT AUDIO DEVICE */

	if(!input_card.empty()){
		input_device_name += ":"+input_card;
	}

	if ((err = snd_pcm_open(&tx_handle, input_device_name.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot open audio device "<<input_device_name << " ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot open input audio device!");
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot allocate hardware parameter structure ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot allocate hardware parameter structure!");
	}

	if ((err = snd_pcm_hw_params_any(tx_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot initialize hardware parameter structure ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot initialize hardware parameter structure for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_access(tx_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set access type ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set access type for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_format(tx_handle, hw_params, format)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample format ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set sample format for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_rate_near(tx_handle, hw_params, &sample_rate, 0)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample rate ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set sample rate for input audio device!");
	}

	if ((err = snd_pcm_hw_params_set_channels(tx_handle, hw_params, 1)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set channel count ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set channel count for input audio device!");
	}

	if ((err = snd_pcm_hw_params(tx_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set parameters ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot set hardware parameters for input audio device!");
	}

	snd_pcm_hw_params_free(hw_params);

	tx_buffer = (char*)malloc(MAX_PAYLOAD_SIZE_H * snd_pcm_format_size(format, 1));

	/* OUTPUT AUDIO DEVICE */

	if(!output_card.empty()){
		output_device_name += ":"+output_card;
	}

	if(!init_pcm_playback(&rx_handle, output_device_name.c_str(), &sample_rate, format)){
		throw CF::Device::InvalidState("Cannot initialize output audio device!");
	}

	//rx_buffer = (char*)malloc(MAX_PAYLOAD_SIZE_H * snd_pcm_format_size(format, 1));

	/* PTT INPUT EVENT DEVICE*/

	ptt_fd = open(ptt_device.c_str(), O_RDONLY);
	if(ptt_fd < 0){
		LOG_ERROR(AudioPortDevice_i, "ptt_device ("<<ptt_device<<") could not be opened " << ptt_fd);
		throw CF::Device::InvalidState("Cannot open ptt input event device!");
	}

	start();
}

bool AudioPortDevice_i::init_pcm_playback(snd_pcm_t **pcm_handle, const char *card_name, unsigned int *sample_rate, snd_pcm_format_t format){

	int err;
	snd_pcm_hw_params_t *hw_params;

	if((err = snd_pcm_open(pcm_handle, card_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0){
		LOG_ERROR(AudioPortDevice_i, "cannot open audio device "<<card_name << " ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot allocate hardware parameter structure ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_any(*pcm_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot initialize hardware parameter structure ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_set_access(*pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set access type ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_set_format(*pcm_handle, hw_params, format)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample format ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(*pcm_handle, hw_params, sample_rate, 0)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample rate ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_set_channels(*pcm_handle, hw_params, 1)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set channel count ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params(*pcm_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set parameters ("<< snd_strerror(err)<<")");
		return false;
	}

	snd_pcm_hw_params_free(hw_params);

	return true;
}

void AudioPortDevice_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	AudioPortDevice_base::start();

	pthread_create(&ptt_thread, NULL, &AudioPortDevice_i::ptt_thread_helper, this);

}

void AudioPortDevice_i::stop() throw (CORBA::SystemException, CF::Resource::StopError) {

	pthread_cancel(ptt_thread);
	pthread_join(ptt_thread, NULL);

	LOG_INFO(AudioPortDevice_i, "ptt thread stopped");

	AudioPortDevice_base::stop();
}

void AudioPortDevice_i::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{

	pthread_mutex_destroy(&tx_lock);
	pthread_mutex_destroy(&tx_stream_lock);

	pthread_mutex_destroy(&rx_lock);

	free(tx_buffer);
	//free(rx_buffer);
	snd_pcm_close(tx_handle);
	snd_pcm_close(rx_handle);

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
	pthread_mutex_lock(&tx_lock);
	pthread_mutex_lock(&rx_lock);

	if(!rx_active && !tx_active){
		setUsageState(CF::Device::IDLE);
	}else if(rx_active || tx_active){
		setUsageState(CF::Device::ACTIVE);
	}else{
		setUsageState(CF::Device::BUSY);
	}

	pthread_mutex_unlock(&tx_lock);
	pthread_mutex_unlock(&rx_lock);
}

int AudioPortDevice_i::serviceFunction()
{
    return FINISH;
}

void AudioPortDevice_i::txThread(){

	CORBA::UShort *buf = (CORBA::UShort *)tx_buffer;
	unsigned int sizeof_frame = sizeof(CORBA::UShort);
	int err, nframes;
	Packet::SeqNum sequenceNumber = 0;

	pthread_mutex_lock(&tx_stream_lock);

	if ((err = snd_pcm_prepare(tx_handle)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot prepare audio interface for use ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot prepare input audio device interface for use!");
	}

	if ((err = snd_pcm_start(tx_handle)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot start PCM stream input device ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot start PCM stream input device!");
	}

	/* BULKIO TEST BEGIN*/
	BULKIO::StreamSRI sri = BULKIO::StreamSRI();
	sri.xdelta = 1.0/sample_rate;
	sri.xunits = BULKIO::UNITS_TIME;
	sri.streamID = "test";
	test_input->pushSRI(sri);
	/* BULKIO TEST END TODO: remove this*/

	// fill buffer a bit.. after pcm starts
	usleep(tx_override_timeout*1e3);

	pthread_mutex_lock(&tx_lock);

	while(tx_active){

		pthread_mutex_unlock(&tx_lock);

		if((nframes = readBuffer(buf, tx_desired_payload, sizeof_frame)) < 0){
			LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
			throw CF::Device::InvalidState("input buffer cannot be read!");
		}

		/* BULKIO TEST BEGIN*/
		if(test_input->isActive()){
			test_input->pushPacket(buf, nframes, bulkio::time::utils::now(), false, "test");
		}
		/* BULKIO TEST END TODO: remove this*/

		if(audio_sample_stream_uses_port->isActive()){
			audio_sample_stream_uses_port->pushPacket(
					(Packet::StreamControlType){false, tx_stream, sequenceNumber, false},
					JTRS::UshortSequence(nframes,nframes,buf,0));
		}

		sequenceNumber++;

		pthread_mutex_lock(&tx_lock);
	}
	pthread_mutex_unlock(&tx_lock);

	// repeat one more time

	if((nframes = readBuffer(buf, tx_desired_payload, sizeof_frame)) < 0){
		LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
		throw CF::Device::InvalidState("input buffer cannot be read!");
	}

	/* BULKIO TEST BEGIN*/
	if(test_input->isActive()){
		test_input->pushPacket(buf, nframes, bulkio::time::utils::now(), true, "test");
	}
	/* BULKIO TEST END TODO: remove this*/

	if(audio_sample_stream_uses_port->isActive()){
		audio_sample_stream_uses_port->pushPacket(
				(Packet::StreamControlType){true, tx_stream, sequenceNumber, tx_abort},
				JTRS::UshortSequence(nframes, tx_abort? 0:nframes, buf, 0));
	}

	tx_stream++;

	snd_pcm_drop(tx_handle);

	pthread_mutex_unlock(&tx_stream_lock);
}

int AudioPortDevice_i::writeBuffer(snd_pcm_t *pcm_handle, const void *vbuffer, int nframes, unsigned sizeof_frame){

	const unsigned char *buffer = (const unsigned char *) vbuffer;
	int ret, orig_nframes;

	orig_nframes = nframes;

	while (nframes > 0) {
		ret = snd_pcm_writei(pcm_handle, buffer, nframes);

		if(ret > 0){

			nframes -= ret;
			buffer += ret * sizeof_frame;

		}else if (ret == -EAGAIN) {
			continue;  // try again
		} else if (ret == -EPIPE) {  // underrun
			fputs("U", stderr);
			if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
				LOG_ERROR(AudioPortDevice_i, "snd_pcm_prepare failed. Can't recover from underrun");
				return ret;
			}
			continue;  // try again

		} else if (ret < 0) {
			LOG_ERROR(AudioPortDevice_i, "snd_pcm_writei failed");
			return ret;
		}
	}

	return orig_nframes;
}

int AudioPortDevice_i::readBuffer(void *vbuffer, int nframes, unsigned sizeof_frame){

	unsigned char *buffer = (unsigned char*) vbuffer;
	int ret, orig_nframes;
	struct timeval start, end, diff;

	orig_nframes = nframes;
	gettimeofday(&start, NULL);

	while (nframes > 0) {

		ret = snd_pcm_readi(tx_handle, buffer, nframes); //read

		if(ret > 0){

			nframes -= ret;
			buffer += ret * sizeof_frame;

			gettimeofday(&end, NULL);
			timersub(&end, &start, &diff);

			if(diff.tv_usec > tx_override_timeout*1e3){
				break;
			}

		}else if (ret == -EAGAIN){
			continue;   // try again
		}else if (ret == -EPIPE) {  // overrun

			fputs("O", stderr);
			if ((ret = snd_pcm_prepare(tx_handle)) < 0) {
				LOG_ERROR(AudioPortDevice_i, "snd_pcm_prepare failed. Can't recover from overrun");
				return ret;
			}
			continue;  // try again

		} else if (ret < 0) {
			LOG_ERROR(AudioPortDevice_i, "snd_pcm_readi failed");
			return ret;
		}

	}

	//LOG_INFO(AudioPortDevice_i, "retval: "<< (orig_nframes - nframes));
	//LOG_INFO(AudioPortDevice_i, "Interval: "<< diff.tv_usec);
	return (orig_nframes - nframes);
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

