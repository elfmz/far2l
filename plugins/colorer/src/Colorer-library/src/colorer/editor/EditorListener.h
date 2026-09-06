#ifndef COLORER_EDITORLISTENER_H
#define COLORER_EDITORLISTENER_H

#include <memory>
/**
 * Listener of BaseEditor events
 */
class EditorListener
{
 public:
  /**
   * Informs EditorListener object about text modification event.
   * All the text becomes invalid after the specified line.
   * @param topLine Topmost modified line of text.
   */
  virtual void modifyEvent(size_t topLine) = 0;

  EditorListener() = default;
  virtual ~EditorListener() = default;
  EditorListener(EditorListener&&) = delete;
  EditorListener(const EditorListener&) = delete;
  EditorListener& operator=(const EditorListener&) = delete;
  EditorListener& operator=(EditorListener&&) = delete;
};

#endif  // COLORER_EDITORLISTENER_H
