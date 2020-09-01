#if defined (__SAMD51__)

#include "samd51_common.hpp"

#undef PORT_PINCFG_PULLEN
#undef PORT_PINCFG_PULLEN_Pos
#undef PORT_PINCFG_INEN
#undef PORT_PINCFG_INEN_Pos

#define _Ul(n) (static_cast<std::uint32_t>((n)))
#define PORT_PINCFG_INEN_Pos        1            /**< \brief (PORT_PINCFG) Input Enable */
#define PORT_PINCFG_INEN            (_Ul(0x1) << PORT_PINCFG_INEN_Pos)
#define PORT_PINCFG_PULLEN_Pos      2            /**< \brief (PORT_PINCFG) Pull Enable */
#define PORT_PINCFG_PULLEN          (_Ul(0x1) << PORT_PINCFG_PULLEN_Pos)


namespace lgfx
{

  void lgfxPinMode(std::uint32_t pin, pin_mode_t mode)
  {
    std::uint32_t port = pin>>8;
    pin &= 0xFF;
    std::uint32_t pinMask = (1ul << pin);

    // Set pin mode according to chapter '22.6.3 I/O Pin Configuration'
    switch ( mode )
    {
      case pin_mode_t::input:
        // Set pin to input mode
        PORT->Group[port].PINCFG[pin].reg=(std::uint8_t)(PORT_PINCFG_INEN) ;
        PORT->Group[port].DIRCLR.reg = pinMask ;
      break ;

      case pin_mode_t::input_pullup:
        // Set pin to input mode with pull-up resistor enabled
        PORT->Group[port].PINCFG[pin].reg=(std::uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
        PORT->Group[port].DIRCLR.reg = pinMask ;

        // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.7 Data Output Value Set')
        PORT->Group[port].OUTSET.reg = pinMask ;
      break ;

      case pin_mode_t::input_pulldown:
        // Set pin to input mode with pull-down resistor enabled
        PORT->Group[port].PINCFG[pin].reg=(std::uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN) ;
        PORT->Group[port].DIRCLR.reg = pinMask ;

        // Enable pull level (cf '22.6.3.2 Input Configuration' and '22.8.6 Data Output Value Clear')
        PORT->Group[port].OUTCLR.reg = pinMask ;
      break ;

      case pin_mode_t::output:
        // enable input, to support reading back values, with pullups disabled
        PORT->Group[port].PINCFG[pin].reg=(std::uint8_t)(PORT_PINCFG_INEN) ;

        // Set pin to output mode
        PORT->Group[port].DIRSET.reg = pinMask ;
      break ;

      default:
        // do nothing
      break ;
    }
  }

//----------------------------------------------------------------------------

  namespace spi // TODO: implement.
  {
    void beginTransaction(int spi_host, int spi_cs, int freq, int spi_mode)
    {}

    void endTransaction(int spi_host, int spi_cs)
    {}

    void writeData(int spi_host, const std::uint8_t* data, std::uint32_t len)
    {}

    void readData(int spi_host, std::uint8_t* data, std::uint32_t len)
    {}
  }

//----------------------------------------------------------------------------

  namespace i2c // TODO: implement.
  {
    void init(int i2c_port, int sda, int scl, int freq) { }

    bool writeRegister(int i2c_port, std::uint16_t addr, std::uint8_t reg, const std::uint8_t *data, uint8_t len)
    {
      return false;
    }

    bool readRegister(int i2c_port, std::uint16_t addr, std::uint8_t reg, std::uint8_t *data, uint8_t len)
    {
      return false;
    }

    bool writeByte(int i2c_port, std::uint16_t addr, std::uint8_t reg, std::uint8_t data, std::uint8_t mask)
    {
      return false;
    }
  }

}

#endif
