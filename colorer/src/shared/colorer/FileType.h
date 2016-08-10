#ifndef _COLORER_FILETYPE_H_
#define _COLORER_FILETYPE_H_

#include<common/Common.h>
#include<colorer/Scheme.h>

class Scheme;

/**
 * HRC FileType (or prototype) instance.
 * @ingroup colorer
 */
class FileType{
public:

  /**
   * Public name of file type (HRC 'name' attribute).
   * @return File type Name
   */
  virtual const String *getName() = 0;

  /**
   * Public group name of file type (HRC 'group' attribute).
   * @return File type Group
   */
  virtual const String *getGroup() = 0;

  /** Public description of file type (HRC 'description' attribute).
      @return File type Description
  */
  virtual const String *getDescription() = 0;

  /** Returns the base scheme of this file type.
      Basically, this is the scheme with same public name, as it's type.
      If this FileType object is not yet loaded, it is loaded with this call.
      @return File type base scheme, to be used as root scheme of text parsing.
  */
  virtual Scheme *getBaseScheme() = 0;

  /** Enumerates all available parameters, defined in this file type.
      @return Parameter name with index <code>idx</code> or <code>null</code>
      if index is too large.
  */
  virtual const String *enumerateParameters(int idx) = 0;

  virtual const String *getParameterDescription(const String &name) = 0;

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
  virtual const String *getParamValue(const String &name) = 0;
  virtual int getParamValueInt(const String &name, int def) = 0;

  /** Returns parameter's default value of this file type.
      Default values are the values, explicitly pointed with
      \c value attribute.
      @param name Parameter's name
      @return Default value of this parameter
  */
  virtual const String *getParamDefaultValue(const String &name) = 0;

  /** Changes value of the parameter with specified name.
      Note, that changed parameter values are not stored in HRC
      base - they remains active only during this HRC session.
      Application should use its own mechanism to save these
      values between sessions (if needed).
      @param name Parameter's name
      @param value New value of this parameter.
  */
  virtual void setParamValue(const String &name, const String *value) = 0;

protected:
  FileType(){};
  virtual ~FileType(){};
};

#endif
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is the Colorer Library.
 *
 * The Initial Developer of the Original Code is
 * Cail Lomecb <cail@nm.ru>.
 * Portions created by the Initial Developer are Copyright (C) 1999-2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */
