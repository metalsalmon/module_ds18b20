#include <DallasTemperature.h>

class DS18B20
{
    private:
        uint32_t iteration_counter;
    public:
        const std::string device_uuid;
        const uint32_t poll_rate;
        DeviceAddress address;

    DS18B20(const std::string device_uuid, const uint32_t poll_rate, const std::string address):
        device_uuid(device_uuid), poll_rate(poll_rate)
    {          
        const char* address_byte = strtok(strdup(address.c_str()), " ");
        for(int i = 0; i < 8; i++)
        {
            this->address[i] = strtol(address_byte, NULL, 16);
            address_byte = strtok(NULL, " ");
        }
        iteration_counter = poll_rate;
    }

    bool decrease_counter()
    {
        iteration_counter--;

        if (iteration_counter == 0) 
        {
            iteration_counter = poll_rate;
            return true;
        }
        return false;
    }
};
