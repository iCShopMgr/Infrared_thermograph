#ifndef eeprom_anything_h
#define eeprom_anything_h

// function prototype
template <class T> int EEPROM_writeAnything(int ee, const T& value);
template <class T> int EEPROM_readAnything(int ee, T& value);

#endif /* eeprom_anything_h */
