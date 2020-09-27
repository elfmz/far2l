#ifndef _COLORER_FILETYPE_H_
#define _COLORER_FILETYPE_H_

#include <vector>
#include <colorer/Common.h>
#include <colorer/Scheme.h>

/**
 * HRC FileType (or prototype) instance.
 * @ingroup colorer
 */
class FileType
{
public:

  /**
   * Public name of file type (HRC 'name' attribute).
   * @return File type Name
   */
  virtual const String* getName() const = 0;

  /**
   * Public group name of file type (HRC 'group' attribute).
   * @return File type Group
   */
  virtual const String* getGroup() const = 0;

  /** Public description of file type (HRC 'description' attribute).
      @return File type Description
  */
  virtual const String* getDescription() const = 0;

  /** Returns the base scheme of this file type.
      Basically, this is the scheme with same public name, as it's type.
      If this FileType object is not yet loaded, it is loaded with this call.
      @return File type base scheme, to be used as root scheme of text parsing.
  */
  virtual Scheme* getBaseScheme() = 0;

  /** Enumerates all available parameters, defined in this file type.
      @return Parameter name with index <code>idx</code> or <code>null</code>
      if index is too large.
  */
  virtual std::vector<SString> enumParams() const = 0;

  virtual const String* getParamDescription(const String &name) const = 0;

  /** Returns parameter's value of this file type.
      Parameters are stored in prototypes as
      <pre>
      \<parameters>
        \<param name="name" value="value" description="..."/>
      \</parameter>
      </pre>
      Parameters can be used to store application
      specific information about each type of file.
      Also parameters are accessible from the HRC definition
      using <code>if/unless</code> attributes of scheme elements.
      This allows portable customization of HRC loading.
      @param name Parameter's name
      @return Value (changed or default) of this parameter
  */
  virtual const String* getParamValue(const String &name) const = 0;
  virtual int getParamValueInt(const String &name, int def) const = 0;

  /** Returns parameter's default value of this file type.
      Default values are the values, explicitly pointed with
      \c value attribute.
      @param name Parameter's name
      @return Default value of this parameter
  */
  virtual const String* getParamDefaultValue(const String &name) const = 0;

  /** Changes value of the parameter with specified name.
      Note, that changed parameter values are not stored in HRC
      base - they remains active only during this HRC session.
      Application should use its own mechanism to save these
      values between sessions (if needed).
      @param name Parameter's name
      @param value New value of this parameter.
  */
  virtual void setParamValue(const String &name, const String* value) = 0;

protected:
  FileType() {};
  virtual ~FileType() {};
};

#endif

