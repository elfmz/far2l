#ifndef _COLORER_EDITORLISTENER_H_
#define _COLORER_EDITORLISTENER_H_

/**
 * Listener of BaseEditor events
 */
class EditorListener {
public:
  /**
   * Informs EditorListener object about text modification event.
   * All the text becomes invalid after the specified line.
   * @param topLine Topmost modified line of text.
   */
  virtual void modifyEvent(size_t topLine) = 0;

};

#endif


