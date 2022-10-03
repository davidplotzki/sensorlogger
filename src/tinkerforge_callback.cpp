#ifdef OPTION_TINKERFORGE

#include "tinkerforge.h"

#include "sensor_tinkerforge.h"
#include "logger.h"
#include <iostream>
tinkerforge_callback::tinkerforge_callback()
{
}

tinkerforge_callback::~tinkerforge_callback()
{
}

bool tinkerforge_callback::valueHasToChange() const
{
	return _value_has_to_change;
}

double tinkerforge_callback::min() const
{
	return _min;
}

double tinkerforge_callback::max() const
{
	return _max;
}

void tinkerforge_callback::setValueHasToChange(bool value_has_to_change)
{
	_value_has_to_change = value_has_to_change;
}

void tinkerforge_callback::setMin(double min)
{
	_min = min;
}

void tinkerforge_callback::setMax(double max)
{
	_max = max;
}



// ####### IO-4 

tinkerforge_callback_io4::tinkerforge_callback_io4(sensorTinkerforge* s, tinkerforge* tinkerMan)
{
	_io4 = NULL;
	_s = s;
	_tinkerMan = tinkerMan;
}

tinkerforge_callback_io4::~tinkerforge_callback_io4()
{
	reset();
}

void tinkerforge_callback_io4::reset()
{
	if(_io4 != NULL)
	{
		io4_destroy(_io4);
		delete _io4;
	}
}

void tinkerforge_callback_io4::registerCallback()
{
	if(ipcon_get_connection_state(_tinkerMan->_ipcon) == IPCON_CONNECTION_STATE_CONNECTED)
	{
		reset();

		_io4 = new IO4();
		io4_create(_io4, _s->getUID().c_str(), _tinkerMan->_ipcon);
		int result = io4_set_debounce_period(_io4, _s->getDebounceTime());
		if(result < 0)
		{
			throw result;
		}

		io4_register_callback(_io4, IO4_CALLBACK_INTERRUPT, (void (*)(void))callback_io4, _s);
		result = io4_set_interrupt(_io4, 15);  // Register callback on all channels. Sort out upon call receival.

		if(result < 0)
		{
			throw result;
		}
	}
}


// ####### IO-4 V2

tinkerforge_callback_io4_v2::tinkerforge_callback_io4_v2(sensorTinkerforge* s, tinkerforge* tinkerMan)
{
	_io4_v2 = NULL;
	_s = s;
	_tinkerMan = tinkerMan;
}

tinkerforge_callback_io4_v2::~tinkerforge_callback_io4_v2()
{
	reset();
}

void tinkerforge_callback_io4_v2::reset()
{
	if(_io4_v2 != NULL)
	{
		io4_v2_destroy(_io4_v2);
		delete _io4_v2;
	}
}

void tinkerforge_callback_io4_v2::registerCallback()
{
	if(ipcon_get_connection_state(_tinkerMan->_ipcon) == IPCON_CONNECTION_STATE_CONNECTED)
	{
		reset();

		_io4_v2 = new IO4V2();
		io4_v2_create(_io4_v2, _s->getUID().c_str(), _tinkerMan->_ipcon);
		int result = io4_v2_set_edge_count_configuration(_io4_v2, _s->getChannel(), IO4_V2_EDGE_TYPE_BOTH, _s->getDebounceTime());
		if(result < 0)
		{
			throw result;
		}

		// Register callback for interrupts
		io4_v2_register_callback(_io4_v2, IO4_V2_CALLBACK_INPUT_VALUE, (void (*)(void))callback_io_v2, this);

		result = io4_v2_set_input_value_callback_configuration(_io4_v2, _s->getChannel(), static_cast<uint32_t>(_s->getMinimumRestPeriod()), true);
		if(result < 0)
		{
			throw result;
		}
	}
}


// ####### IO-16 

tinkerforge_callback_io16::tinkerforge_callback_io16(sensorTinkerforge* s, tinkerforge* tinkerMan)
{
	_io16 = NULL;
	_s = s;
	_tinkerMan = tinkerMan;
}

tinkerforge_callback_io16::~tinkerforge_callback_io16()
{
	reset();
}

void tinkerforge_callback_io16::reset()
{
	if(_io16 != NULL)
	{
		io16_destroy(_io16);
		delete _io16;
	}
}

void tinkerforge_callback_io16::registerCallback()
{
	if(ipcon_get_connection_state(_tinkerMan->_ipcon) == IPCON_CONNECTION_STATE_CONNECTED)
	{
		reset();

		_io16 = new IO16();
		io16_create(_io16, _s->getUID().c_str(), _tinkerMan->_ipcon);
		int result = io16_set_debounce_period(_io16, _s->getDebounceTime());
		if(result < 0)
		{
			throw result;
		}

		// Register callback for interrupts
		io16_register_callback(_io16, IO16_CALLBACK_INTERRUPT, (void (*)(void))callback_io16, this);

		// Enable interrupt on all pins. Bit mask will be applied upon callback.
		result = io16_set_port_interrupt(_io16, _s->getIOPort(), 15);
		if(result < 0)
		{
			throw result;
		}
	}
}

// ####### IO-16 V2

tinkerforge_callback_io16_v2::tinkerforge_callback_io16_v2(sensorTinkerforge* s, tinkerforge* tinkerMan)
{
	_io16_v2 = NULL;
	_s = s;
	_tinkerMan = tinkerMan;
}

tinkerforge_callback_io16_v2::~tinkerforge_callback_io16_v2()
{
	reset();
}

void tinkerforge_callback_io16_v2::reset()
{
	if(_io16_v2 != NULL)
	{
		io16_v2_destroy(_io16_v2);
		delete _io16_v2;
	}
}

void tinkerforge_callback_io16_v2::registerCallback()
{
	if(ipcon_get_connection_state(_tinkerMan->_ipcon) == IPCON_CONNECTION_STATE_CONNECTED)
	{
		reset();

		_io16_v2 = new IO16V2();
		io16_v2_create(_io16_v2, _s->getUID().c_str(), _tinkerMan->_ipcon);
		int result = io16_v2_set_edge_count_configuration(_io16_v2, _s->getChannel(), IO4_V2_EDGE_TYPE_BOTH, _s->getDebounceTime());
		if(result < 0)
		{
			throw result;
		}

		// Register callback for interrupts
		io16_v2_register_callback(_io16_v2, IO16_V2_CALLBACK_INPUT_VALUE, (void (*)(void))callback_io_v2, this);

		// Enable interrupt on all pins:
		result = io16_v2_set_input_value_callback_configuration(_io16_v2, _s->getChannel(), static_cast<uint32_t>(_s->getMinimumRestPeriod()), true);
		if(result < 0)
		{
			throw result;
		}
	}
}


// For IO-4 Bricklet
void callback_io4(uint8_t interrupt_mask, uint8_t value_mask, sensorTinkerforge* s)
{
	// We have to find all other sensors with this UID,
	// because only one callback is registered per UID.

	for(size_t i=0; i<s->ptLogger()->nSensors(); ++i)
	{
		sensor* sensori = s->ptLogger()->getSensor(i);
		if(sensori->type() == sensor_tinkerforge)
		{
			sensorTinkerforge* tfSensor = dynamic_cast<sensorTinkerforge*>(sensori);

			if(tfSensor->getUID() == s->getUID())
			{
				if(interrupt_mask == tfSensor->getBitMask())
				{
					int value = value_mask & tfSensor->getBitMask();			
					if(value == 0)	// Input is set to low=0 upon switching.
					{
						if((tfSensor->getTriggerEvent() == low) || (tfSensor->getTriggerEvent() == high_or_low))
							tfSensor->addRawMeasurement(0);
					}
					else  // high
					{
						if((tfSensor->getTriggerEvent() == high) || (tfSensor->getTriggerEvent() == high_or_low))
							tfSensor->addRawMeasurement(1);
					}
				}
			}
		}
	}
}

// For IO-16 Bricklet
void callback_io16(char port, uint8_t interrupt_mask, uint8_t value_mask, sensorTinkerforge* s)
{
	// We have to find all other sensors with this UID,
	// because only one callback is registered per UID.

	for(size_t i=0; i<s->ptLogger()->nSensors(); ++i)
	{
		sensor* sensori = s->ptLogger()->getSensor(i);
		if(sensori->type() == sensor_tinkerforge)
		{
			sensorTinkerforge* tfSensor = dynamic_cast<sensorTinkerforge*>(sensori);

			if(tfSensor->getUID() == s->getUID())
			{
				if(port == tfSensor->getIOPort())
				{
					if(interrupt_mask == tfSensor->getBitMask())
					{
						int value = value_mask & tfSensor->getBitMask();			
						if(value == 0)	// Input is set to low=0 upon switching.
						{
							if((tfSensor->getTriggerEvent() == low) || (tfSensor->getTriggerEvent() == high_or_low))
								tfSensor->addRawMeasurement(0);	
						}
						else  // high
						{
							if((tfSensor->getTriggerEvent() == high) || (tfSensor->getTriggerEvent() == high_or_low))
								tfSensor->addRawMeasurement(1);	
						}
					}
				}
			}
		}
	}
}

// For IO-4 2.0 and IO-16 2.0 Bricklet
void callback_io_v2(uint8_t channel, bool changed, bool value, sensorTinkerforge* s)
{
	// We have to find all other sensors with this UID,
	// because only one callback is registered per UID.

	for(size_t i=0; i<s->ptLogger()->nSensors(); ++i)
	{
		sensor* sensori = s->ptLogger()->getSensor(i);
		if(sensori->type() == sensor_tinkerforge)
		{
			sensorTinkerforge* tfSensor = dynamic_cast<sensorTinkerforge*>(sensori);

			if(tfSensor->getUID() == s->getUID())
			{
				if(channel == tfSensor->getChannel())
				{
					if(value == false)	// Input is set to low=0 upon switching.
					{
						if((tfSensor->getTriggerEvent() == low) || (tfSensor->getTriggerEvent() == high_or_low))
							tfSensor->addRawMeasurement(0);	
					}
					else  // high
					{
						if((tfSensor->getTriggerEvent() == high) || (tfSensor->getTriggerEvent() == high_or_low))
							tfSensor->addRawMeasurement(1);	
					}
				}
			}
		}
	}
}

#endif