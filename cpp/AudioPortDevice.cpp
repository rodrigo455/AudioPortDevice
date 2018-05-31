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

PREPARE_LOGGING(AudioPortDevice_i)

ToneControl::ToneControl(Audio::AudibleAlertsAndAlarms::ToneProfileType profile, AudioPortDevice_i* dev):profile(profile){
	status = false;
	pthread_mutex_init(&lock, NULL);
	thread = 0;
	audio_device = static_cast<AudioPortDevice_i *>(dev);
}

ToneControl::~ToneControl(){
	stop();
	pthread_mutex_destroy(&lock);
}

void ToneControl::start(){
	pthread_mutex_lock(&lock);
	status = true;
	pthread_mutex_unlock(&lock);
	if(profile._d() == Audio::AudibleAlertsAndAlarms::SIMPLE_TONE){
		pthread_create(&thread, NULL, &ToneControl::simple_tone_thread_helper, this);
	}else{
		pthread_create(&thread, NULL, &ToneControl::complex_tone_thread_helper, this);
	}
}

void ToneControl::stop(){
	pthread_mutex_lock(&lock);
	status = false;
	pthread_mutex_unlock(&lock);
	pthread_join(thread, NULL);
}

CORBA::ULong ToneControl::getNumSamples(){
	if(profile._d() == Audio::AudibleAlertsAndAlarms::SIMPLE_TONE){
		return 0;
	}else{
		return profile.complexTone().toneSamples.length();
	}
}

void ToneControl::simple_tone_thread(){

	short *buffer;
	CORBA::UShort sleep_interval;
	int nsamples, orig_nsamples, buffersize;
	int write_samples = 0;
	unsigned int sample_rate = TONE_SAMPLE_RATE;
	float phase = 0;
	float delta_phase = 0;
	snd_pcm_t *tone_handle;

	const Audio::AudibleAlertsAndAlarms::SimpleToneProfile simple = profile.simpleTone();

	orig_nsamples = simple.durationPerBurstInMs* (sample_rate/1e3);
	nsamples = orig_nsamples;
	buffersize = (orig_nsamples>TONE_SAMPLE_RATE)? TONE_SAMPLE_RATE:orig_nsamples;
	buffer = (short*)malloc(buffersize * sizeof(short));
	sleep_interval = simple.repeatIntervalInMs - simple.durationPerBurstInMs;

	AudioPortDevice_i::init_pcm(
			&tone_handle,
			audio_device->output_device_name.c_str(),
			SND_PCM_STREAM_PLAYBACK,
			&sample_rate,
			SND_PCM_FORMAT_S16_LE,
			0);

	delta_phase = 2*M_PI*simple.frequencyInHz/sample_rate;
	for(int i = 0; i < buffersize; i++){
		phase += delta_phase;
		buffer[i] = 2000*cosf(phase);
	}

	if(sleep_interval){

		pthread_mutex_lock(&lock);
		while(status){
			pthread_mutex_unlock(&lock);

			if (snd_pcm_prepare(tone_handle) < 0) {
				free(buffer);
				snd_pcm_close(tone_handle);
				LOG_ERROR(AudioPortDevice_i, "Could not start tone");
				return;
			}
			nsamples = orig_nsamples;
			write_samples = 0;

			pthread_mutex_lock(&lock);
			while(status && write_samples!=nsamples){
				pthread_mutex_unlock(&lock);

				if(nsamples > TONE_SAMPLE_RATE){
					write_samples = TONE_SAMPLE_RATE;
					nsamples -= TONE_SAMPLE_RATE;
				}else{
					write_samples = nsamples;
				}

				AudioPortDevice_i::writeBuffer(tone_handle, buffer, write_samples, sizeof(short));

				pthread_mutex_lock(&lock);
			}
			pthread_mutex_unlock(&lock);

			snd_pcm_drain(tone_handle);

			if(!simple.repeatIntervalInMs){
				pthread_mutex_lock(&lock);
				break;
			}

			usleep(sleep_interval*1e3);

			pthread_mutex_lock(&lock);
		}
		status = false; // ensure status false
		pthread_mutex_unlock(&lock);

	}else{

		if (snd_pcm_prepare(tone_handle) < 0) {
			free(buffer);
			snd_pcm_close(tone_handle);
			LOG_ERROR(AudioPortDevice_i, "Could not start tone");
			return;
		}

		pthread_mutex_lock(&lock);
		while(status){

			nsamples = orig_nsamples;
			write_samples = 0;

			while(status && write_samples!=nsamples){
				pthread_mutex_unlock(&lock);

				if(nsamples > TONE_SAMPLE_RATE){
					write_samples = TONE_SAMPLE_RATE;
					nsamples -= TONE_SAMPLE_RATE;
				}else{
					write_samples = nsamples;
				}

				AudioPortDevice_i::writeBuffer(tone_handle, buffer, write_samples, sizeof(short));

				pthread_mutex_lock(&lock);
			}
		}
		status = false; // ensure status false
		pthread_mutex_unlock(&lock);

		snd_pcm_drain(tone_handle);
	}

	snd_pcm_close(tone_handle);
	free(buffer);
}

void ToneControl::complex_tone_thread(){
	snd_pcm_t *tone_handle;
	unsigned int sample_rate = TONE_SAMPLE_RATE;
	const Audio::AudibleAlertsAndAlarms::ComplexToneProfile complex = profile.complexTone();
	CORBA::UShort repeat = complex.numberOfRepeats;

	AudioPortDevice_i::init_pcm(
			&tone_handle,
			audio_device->output_device_name.c_str(),
			SND_PCM_STREAM_PLAYBACK,
			&sample_rate,
			SND_PCM_FORMAT_S16_LE,
			0);

	if (snd_pcm_prepare(tone_handle) < 0) {
		snd_pcm_close(tone_handle);
		LOG_ERROR(AudioPortDevice_i, "Could not start tone");
		return;
	}

	pthread_mutex_lock(&lock);
	while(status && (repeat--)){
		pthread_mutex_unlock(&lock);

		AudioPortDevice_i::writeBuffer(
				tone_handle,
				complex.toneSamples.get_buffer(),
				complex.toneSamples.length(),
				sizeof(short));

		pthread_mutex_lock(&lock);
	}
	status = false; // ensure status false
	pthread_mutex_unlock(&lock);

	snd_pcm_drop(tone_handle);
	snd_pcm_close(tone_handle);
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl)
{
	construct();
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, compDev)
{
	construct();
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, capacities)
{
	construct();
}

AudioPortDevice_i::AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev) :
    AudioPortDevice_base(devMgr_ior, id, lbl, sftwrPrfl, capacities, compDev)
{
	construct();
}

AudioPortDevice_i::~AudioPortDevice_i()
{
}

void AudioPortDevice_i::construct(){
	// To get card names run the command:
	// aplay -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
	// arecord -l | awk -F \: '/,/{print $2}' | awk '{print $1}' | uniq
	input_device_name = "plughw";
	output_device_name = "plug:dmix";

	tx_active = false;
	tx_abort = false;
	rx_active = false;

	tx_stream = 0;

	tx_override_timeout = 23;
	tx_payload_size = 320;

	pthread_mutex_init(&tx_lock, NULL);
	pthread_mutex_init(&tx_stream_lock, NULL);

	pthread_mutex_init(&rx_lock, NULL);
}

void AudioPortDevice_i::constructor()
{
    /***********************************************************************************
     This is the RH constructor. All properties are properly initialized before this function is called
    ***********************************************************************************/

	tx_buffer = (char*)malloc(MAX_PAYLOAD_SIZE_H * sizeof(CORBA::UShort));

	if(!capture_card.empty())
		input_device_name += ":"+capture_card;

	if(!playback_card.empty())
		output_device_name += ":"+playback_card;

	ptt_fd = open(ptt_device.c_str(), O_RDONLY);
	if(ptt_fd < 0){
		LOG_ERROR(AudioPortDevice_i, "ptt_device ("<<ptt_device<<") could not be opened " << ptt_fd);
		throw CF::Device::InvalidState("Cannot open ptt input event device!");
	}

	playbackVolumeChanged(0,playback_volume);
	captureVolumeChanged(0,capture_volume);

	addPropertyListener(playback_volume, this, &AudioPortDevice_i::playbackVolumeChanged);
	addPropertyListener(capture_volume, this, &AudioPortDevice_i::captureVolumeChanged);

	start();
}

CORBA::ULong AudioPortDevice_i::init_pcm(snd_pcm_t **pcm_handle, const char *card_name, snd_pcm_stream_t stream, unsigned int *sample_rate, snd_pcm_format_t format, int mode){

	int err, dir = 0;
	unsigned long int period_size;
	unsigned int min_nperiods, max_nperiods;
	snd_pcm_hw_params_t *hw_params;

	if((err = snd_pcm_open(pcm_handle, card_name, stream, mode)) < 0){
		LOG_ERROR(AudioPortDevice_i, "cannot open audio device "<<card_name << " ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot allocate hardware parameters structure ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_any(*pcm_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot initialize hardware parameter structure ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_access(*pcm_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set access type ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_format(*pcm_handle, hw_params, format)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample format ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_set_rate_near(*pcm_handle, hw_params, sample_rate, 0)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set sample rate ("<< snd_strerror(err)<<")");
		return false;
	}

	if ((err = snd_pcm_hw_params_set_channels(*pcm_handle, hw_params, 1)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set channel count ("<< snd_strerror(err)<<")");
		return 0;
	}

	snd_pcm_hw_params_get_periods_min(hw_params, &min_nperiods, NULL);
	snd_pcm_hw_params_get_periods_max(hw_params, &max_nperiods, NULL);

	if ((err = snd_pcm_hw_params(*pcm_handle, hw_params)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot set hardware parameters ("<< snd_strerror(err)<<")");
		return 0;
	}

	if ((err = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot get period_size ("<< snd_strerror(err)<<")");
		return 0;
	}

	snd_pcm_hw_params_free(hw_params);

	if(stream == SND_PCM_STREAM_PLAYBACK){
		snd_pcm_sw_params_t *sw_params;

		if ((err = snd_pcm_sw_params_malloc(&sw_params)) < 0) {
			LOG_ERROR(AudioPortDevice_i, "cannot allocate software parameters structure ("<< snd_strerror(err)<<")");
			return 0;
		}

		if ((err = snd_pcm_sw_params_current(*pcm_handle, sw_params)) < 0) {
			LOG_ERROR(AudioPortDevice_i, "cannot initialize software parameters structure ("<< snd_strerror(err)<<")");
			return 0;
		}

		/* Wait to start until buffer is half way full to avoid audio underruns */
		if ((err = snd_pcm_sw_params_set_start_threshold(*pcm_handle, sw_params, (min_nperiods+max_nperiods)*period_size/4))){
			LOG_ERROR(AudioPortDevice_i, "cannot set start threshold ("<< snd_strerror(err)<<")");
			return 0;
		}

		if ((err = snd_pcm_sw_params(*pcm_handle, sw_params)) < 0) {
			LOG_ERROR(AudioPortDevice_i, "cannot set software parameters ("<< snd_strerror(err)<<")");
			return 0;
		}

		snd_pcm_sw_params_free(sw_params);
	}

	return period_size;
}

void AudioPortDevice_i::start() throw (CORBA::SystemException, CF::Resource::StartError) {
	AudioPortDevice_base::start();

	pthread_create(&ptt_thread, NULL, &AudioPortDevice_i::ptt_thread_helper, this);

}

void AudioPortDevice_i::stop() throw (CORBA::SystemException, CF::Resource::StopError) {

	pthread_mutex_lock(&tx_lock);

	pthread_cancel(ptt_thread);
	pthread_join(ptt_thread, NULL);

	pthread_mutex_unlock(&tx_lock);

	audio_alertalarm_wf_provides_port->stopAllTones();

	LOG_DEBUG(AudioPortDevice_i, "ptt thread stopped");

	AudioPortDevice_base::stop();
}

void AudioPortDevice_i::releaseObject() throw (CORBA::SystemException, CF::LifeCycle::ReleaseError)
{

	pthread_mutex_destroy(&tx_lock);
	pthread_mutex_destroy(&tx_stream_lock);

	pthread_mutex_destroy(&rx_lock);

	free(tx_buffer);

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

	if(!(tx_payload_size = AudioPortDevice_i::init_pcm(
				&tx_handle,
				input_device_name.c_str(),
				SND_PCM_STREAM_CAPTURE,
				&sample_rate,
				SND_PCM_FORMAT_U16_LE,
				SND_PCM_NONBLOCK))){
		throw CF::Device::InvalidState("Cannot initialize input audio device!");
	}else{
		tx_override_timeout = ((tx_payload_size*1000)/sample_rate)+5;
	}

	if(audio_sample_stream_uses_port->isActive()){
		tx_payload_size = audio_sample_stream_uses_port->getDesiredPayloadSize();
		tx_override_timeout = audio_sample_stream_uses_port->getMinOverrideTimeout();
	}

	if ((err = snd_pcm_prepare(tx_handle)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot prepare audio interface for use ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot prepare input audio device interface for use!");
	}

	if ((err = snd_pcm_start(tx_handle)) < 0) {
		LOG_ERROR(AudioPortDevice_i, "cannot start PCM stream input device ("<< snd_strerror(err)<<")");
		throw CF::Device::InvalidState("Cannot start PCM stream input device!");
	}

	/* BULKIO TEST BEGIN
	BULKIO::StreamSRI sri = BULKIO::StreamSRI();
	sri.xdelta = 1.0/sample_rate;
	sri.xunits = BULKIO::UNITS_TIME;
	sri.streamID = "test";
	test_input->pushSRI(sri);
	 BULKIO TEST END TODO: remove this*/

	// fill buffer a bit.. after pcm starts
	usleep(tx_override_timeout*1e3);

	pthread_mutex_lock(&tx_lock);

	while(tx_active){

		pthread_mutex_unlock(&tx_lock);

		if((nframes = readBuffer(buf, tx_payload_size, sizeof_frame)) < 0){
			LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
			throw CF::Device::InvalidState("input buffer cannot be read!");
		}

		/* BULKIO TEST BEGIN
		if(test_input->isActive()){
			test_input->pushPacket(buf, nframes, bulkio::time::utils::now(), false, "test");
		}
		 BULKIO TEST END TODO: remove this*/

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

	if((nframes = readBuffer(buf, tx_payload_size, sizeof_frame)) < 0){
		LOG_ERROR(AudioPortDevice_i, "input buffer cannot be read!");
		throw CF::Device::InvalidState("input buffer cannot be read!");
	}

	/* BULKIO TEST BEGIN
	if(test_input->isActive()){
		test_input->pushPacket(buf, nframes, bulkio::time::utils::now(), true, "test");
	}
	 BULKIO TEST END TODO: remove this*/

	if(audio_sample_stream_uses_port->isActive()){
		audio_sample_stream_uses_port->pushPacket(
				(Packet::StreamControlType){true, tx_stream, sequenceNumber, tx_abort},
				JTRS::UshortSequence(nframes, tx_abort? 0:nframes, buf, 0));
	}

	tx_stream++;

	snd_pcm_drop(tx_handle);
	snd_pcm_close(tx_handle);

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

		}else if (ret == -EAGAIN){
			continue;  /* try again */
		} else if (ret == -EPIPE){
			fputs("U", stderr); /* underrun */
			if ((ret = snd_pcm_prepare(pcm_handle)) < 0) {
				LOG_ERROR(AudioPortDevice_i, "snd_pcm_prepare failed. Can't recover from underrun");
				return ret;
			}
			continue;  /* try again */

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
			continue; /* try again */
		}else if (ret == -EPIPE) {  /* overrun */
			fputs("O", stderr);
			if ((ret = snd_pcm_prepare(tx_handle)) < 0) {
				LOG_ERROR(AudioPortDevice_i, "snd_pcm_prepare failed. Can't recover from overrun");
				return ret;
			}
			continue; /* try again */

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
				//LOG_DEBUG(AudioPortDevice_i, "PTT Released");
				pthread_mutex_lock(&tx_lock);
				tx_active = false;
				pthread_mutex_unlock(&tx_lock);
				pthread_join(tx_thread, NULL);

				if(audio_ptt_uses_port->isActive()){
					audio_ptt_uses_port->setPTT(false);
				}

			}else if(event.value == EV_PRESSED){
				//LOG_DEBUG(AudioPortDevice_i, "PTT Pressed");
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

void AudioPortDevice_i::captureVolumeChanged(CORBA::ULong old_value, CORBA::ULong new_value){

	int err;
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	err = snd_mixer_attach(handle, capture_card.empty()? "default":("hw:"+capture_card).c_str());

	if(err<0){
		LOG_ERROR(AudioPortDevice_i, "Could not attach capture device to mixer handle");
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, capture_mixer_control.empty()? "Capture":capture_mixer_control.c_str());
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	if(!elem){
		LOG_ERROR(AudioPortDevice_i, "Could not find mixer control for capture card!");
	}else{
		snd_mixer_selem_get_capture_volume_range(elem, &min, &max);
		snd_mixer_selem_set_capture_volume_all(elem, new_value * max / 100);
	}

	snd_mixer_close(handle);
}

void AudioPortDevice_i::playbackVolumeChanged(CORBA::ULong old_value, CORBA::ULong new_value){

	int err;
	long min, max;
	snd_mixer_t *handle;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&handle, 0);
	err = snd_mixer_attach(handle, playback_card.empty()? "default":("hw:"+playback_card).c_str());

	if(err<0){
		LOG_ERROR(AudioPortDevice_i, "Could not attach playback device to mixer handle");
		snd_mixer_close(handle);
		return;
	}

	snd_mixer_selem_register(handle, NULL, NULL);
	snd_mixer_load(handle);

	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, playback_mixer_control.empty()? "Master":playback_mixer_control.c_str());
	snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

	if(!elem){
		LOG_ERROR(AudioPortDevice_i, "Could not find mixer control for playback card!");
	}else{
		snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
		snd_mixer_selem_set_playback_volume_all(elem, new_value * max / 100);
	}

	snd_mixer_close(handle);
}

