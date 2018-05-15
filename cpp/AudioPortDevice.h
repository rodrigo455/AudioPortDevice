#ifndef AUDIOPORTDEVICE_I_IMPL_H
#define AUDIOPORTDEVICE_I_IMPL_H

#include "AudioPortDevice_base.h"

class AudioPortDevice_i : public AudioPortDevice_base
{
    ENABLE_LOGGING
    public:
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, char *compDev);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities);
        AudioPortDevice_i(char *devMgr_ior, char *id, char *lbl, char *sftwrPrfl, CF::Properties capacities, char *compDev);
        ~AudioPortDevice_i();

        void constructor();

        int serviceFunction();

    protected:
        void updateUsageState();
};

#endif // AUDIOPORTDEVICE_I_IMPL_H
