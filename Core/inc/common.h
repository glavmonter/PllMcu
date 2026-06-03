#ifndef INC_COMMON_H_
#define INC_COMMON_H_

#include <stdint.h>
#include "etl/vector.h"
#include "etl/function.h"


#define PIN_SET(port, pin)   do { port->BSRR = pin; } while (0)
#define PIN_RESET(port, pin) do { port->BRR  = pin; } while (0)


void delay_us(uint32_t us);

void asort(etl::ivector<uint16_t> &v);


union _u8 {
    uint8_t dw;
    struct {
        uint16_t  b0: 1;
        uint16_t  b1: 1;
        uint16_t  b2: 1;
        uint16_t  b3: 1;
        uint16_t  b4: 1;
        uint16_t  b5: 1;
        uint16_t  b6: 1;
        uint16_t  b7: 1;
    };
};


#define FLOAT_BUFFER_SIZE                   16
char *_float_to_char(float x, char *p);


namespace etl_ext {


template <typename TObject, typename TParameter, void (TObject::*Function)(TParameter)>
class function_mpval : public etl::ifunction<void>
{
public:

  typedef TObject    object_type;    ///< The type of object.
  typedef TParameter parameter_type; ///< The type of parameter sent to the function.

  //*************************************************************************
  /// Constructor.
  ///\param object    Reference to the object
  //*************************************************************************
  explicit function_mpval(TObject& object_, TParameter val)
    : p_object(&object_), _val(val)
  {
  }

  //*************************************************************************
  /// The function operator that calls the destination function.
  ///\param data The data to pass to the function.
  //*************************************************************************
  virtual void operator ()() const ETL_OVERRIDE
  {
    // Call the object's member function with the data.
    (p_object->*Function)(_val);
  }

private:
  TObject* p_object; ///< Pointer to the object that contains the function.
  TParameter _val;
};

}

#endif /* INC_COMMON_H_ */
