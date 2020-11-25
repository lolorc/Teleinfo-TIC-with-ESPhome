#include "esphome.h"
#include "esphome/core/defines.h"
#include "esphome/components/text_sensor/text_sensor.h"

class MyTicComponent : public PollingComponent, public UARTDevice, public Switch {
 public:
	MyTicComponent(UARTComponent *parent) : PollingComponent(1000), UARTDevice(parent) {}

	Sensor *sensor_IINST = new Sensor();
	Sensor *sensor_ISOUSC = new Sensor();
	Sensor *sensor_PAPP = new Sensor();
	Sensor *sensor_BASE = new Sensor();
	Sensor *sensor_ERRORS = new Sensor();
	TextSensor *sensor_ADCO = new TextSensor();

	bool enable = true;
	float iinst = 0.0;
	float isousc = 0.0;
	float papp = 0.0;
	float base = 0.0;
	unsigned long errors = 0;
	String adco = "";
	unsigned char chksum = 0;
	unsigned int len = 0;
  
	static MyTicComponent* instance(UARTComponent *parent)
	{
		static MyTicComponent* INSTANCE = new MyTicComponent(parent);
		return INSTANCE;
	}

	void write_state(bool state) override
	{
		enable = state;
		publish_state(state);
	}
	
	void setup() override {
		publish_state(enable);
	}
	
	void update() override {
		String buff = "";
		chksum = 0;
		while (available()>0)
		{
			char c = read();
			// le composant UART reçoit en 8bits, on converti en 7bits  -> Mod by schmurtz : 
			// no more useful since ESPhome Uart improvements : https://github.com/esphome/esphome/commit/fb2b7ade41dc3f5fae8a68e034b6506bf5902b0b
			//c &= 0x7f;
			
			// \r = fin d'un message, on sort de la boucle pour traiter le message
			if (c == '\r')
				break;
			buff += c;
			
			// \n = début d'un message, on vide le buffer
			if (c == '\n' || buff.length() > 50){
				//ESP_LOGI("Buffer", "Buffer Size :  %d", buff.length());
				if (buff.length() > 50){
					ESP_LOGW("Buffer", "Buffer was too big, cleaned !!!");
				}
				buff = "";
				
			}
		}
		
		if (enable && (buff != ""))
		{
			if (buff.length() >= 7) // 4 (plus courte etiquette) + 2 (separateurs) + 1 (valeur)
				processString(buff);
			//ESP_LOGI("Buffer", "Buffer Size :  %d", buff.length());
			buff = "";
		}
	}
	
	void processString(String str) {
		//ESP_LOGD("tic_received", str.c_str());
		chksum = 0;
		len = str.length();
		for(int i = 0; i < len - 2; i++) { // -2 car je retire le separateur et le checksum qui ne comptent pas dans le calcul du checksum
			chksum += str[i];
		}
		chksum = ( chksum & 0x3F ) + 0x20;
		if ( chksum == str[len-1] ) {
			char separator = ' ';
			if (str.indexOf(separator) > -1) {
				String etiquette = str.substring(0, str.indexOf(separator));
				String value = str.substring(str.indexOf(separator) + 1);
				if (value.indexOf(separator) > -1) {
					value = value.substring(0, value.indexOf(separator));
					processCommand(etiquette, value);
				}
			}
		} else {
			errors++;
			ESP_LOGD("tic", "WRONG CHECKSUM, received: '%s', calculated checksum : '%c'", str.c_str(), chksum);
			sensor_ERRORS->publish_state(errors);
		}
	}
  
	void processCommand(String etiquette, String value)
	{
		ESP_LOGD("tic", "tic %s : %s ", etiquette.c_str(), value.c_str());
		if (etiquette == "ADCO") // adresse
		{
			if (adco != value)
			{
				sensor_ADCO->publish_state(value.c_str());
				adco = value;
			}
		}
		else if (etiquette == "BASE")
		{
			if (base != value.toFloat())
			{	
				sensor_BASE->publish_state(value.toFloat());
				base = value.toFloat();
			}
		}
		else if (etiquette == "ISOUSC")
		{
			if (isousc != value.toFloat())
			{	
				sensor_ISOUSC->publish_state(value.toFloat());
				isousc = value.toFloat();
			}
		}
		else if (etiquette == "IINST")
		{
			if (iinst != value.toFloat())
			{
				sensor_IINST->publish_state(value.toFloat());
				iinst = value.toFloat();
			}
		}
		else if (etiquette == "PAPP")
		{
			if (papp != value.toFloat())
			{
				sensor_PAPP->publish_state(value.toFloat());
				papp = value.toFloat();
			}
		}
	}
};
