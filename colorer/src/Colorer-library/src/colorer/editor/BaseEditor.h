#ifndef _COLORER_BASEEDITOR_H_
#define _COLORER_BASEEDITOR_H_

#include <colorer/parsers/ParserFactory.h>
#include <colorer/handlers/LineRegionsSupport.h>
#include <colorer/handlers/LineRegionsCompactSupport.h>
#include <colorer/editor/EditorListener.h>
#include <colorer/editor/PairMatch.h>

/**
 * Base Editor functionality.
 * This class implements basic functionality,
 * which could be useful in application's editing system.
 * This includes automatic top-level caching of hilighting
 * state, outline structure creation, pair constructions search.
 * This class has event-oriented structure. Each editor event
 * is passed into this object and gets internal processing.
 * @ingroup colorer_editor
 */
class BaseEditor : public RegionHandler
{
public:
  /**
   * Initial constructor.
   * Creates uninitialized base editor functionality support.
   * @param pf ParserFactory, used as source of all created
   *        parsers (HRC, HRD, Text parser). Can't be null.
   * @param lineSource Object, that provides parser with
   *        text data in line-separated form. Can't be null.
   */
  BaseEditor(ParserFactory* pf, LineSource* lineSource);
  ~BaseEditor();

  /**
   * This method informs handler about internal form of
   * requeried LineRegion lists, which is returned after the parsing
   * process. Compact regions are guaranteed not to overlap
   * with each other (this is achieved with more internal processing
   * and more extensive cpu usage); non-compact regions are placed directly
   * as they created by the TextParser and can be overlapped.
   * @note By default, if method is not called, regions are not compacted.
   * @param compact Creates LineRegionsSupport (false) or LineRegionsCompactSupport (true)
   *        object to store lists of RegionDefine's
   */
  void setRegionCompact(bool compact);

  /**
   * Installs specified RegionMapper, which
   * maps HRC Regions into color data.
   * @param rm RegionMapper object to map region values into colors.
   */
  void setRegionMapper(RegionMapper* rm);

  /**
   * Installs specified RegionMapper, which
   * is created with ParserFactory methods and maintained internally by this handler.
   * If no one of two overloads of setRegionMapper is called,
   * all work is started without mapping of extended region information.
   * @param hrdClass Class of RegionMapper instance
   * @param hrdName  Name of RegionMapper instance
   */
  void setRegionMapper(const String* hrdClass, const String* hrdName);

  /**
   * Specifies number of lines, for which parser
   * would be able to run continual processing without
   * highlight invalidation.
   * @param backParse Number of lines. If <= 0, dropped into default
   * value.
   */
  void setBackParse(int backParse);

  /**
   * Initial HRC type, used for parse processing.
   * If changed during processing, all text information
   * is invalidated.
   */
  void setFileType(FileType* ftype);
  /**
   * Initial HRC type, used for parse processing.
   * If changed during processing, all text information is invalidated.
   */
  FileType* setFileType(const String& fileType);
  /**
   * Tries to choose appropriate file type from HRC database
   * using passed fileName and first line of text (if available through lineSource)
   */
  FileType* chooseFileType(const String* fileName);

  /**
   * Returns currently used HRC file type
   */
  FileType* getFileType();

  /**
   * Adds specified RegionHandler object
   * into parse process.
   */
  void addRegionHandler(RegionHandler* rh);

  /**
   * Removes previously added RegionHandler object.
   */
  void removeRegionHandler(RegionHandler* rh);

  /**
   * Adds specified EditorListener object into parse process.
   */
  void addEditorListener(EditorListener* el);

  /**
   * Removes previously added EditorListener object.
   */
  void removeEditorListener(EditorListener* el);

  /**
   * Searches and creates pair match object in currently visible text.
   * @param lineNo Line number, where to search paired region.
   * @param pos Position in line, where paired region to be searched.
   *        Paired Region is found, if it includes specified position
   *        or ends directly at one char before line position.
  */
  PairMatch* searchLocalPair(int lineNo, int pos);

  /**
   * Searches pair match in all available text, possibly,
   * making additional processing.
   * @param pos Position in line, where paired region to be searched.
   *        Paired Region is found, if it includes specified position
   *        or ends directly at one char before line position.
   */
  PairMatch* searchGlobalPair(int lineNo, int pos);

  /**
   * Searches and creates pair match object of first enwrapping block.
   * Returned object could be used as with getPairMatch method.
   * Enwrapped block is the first meeted start of block, if moving
   * from specified position to the left and top.
   * Not Implemented yet.
   *
   * @param lineNo Line number, where to search paired region.
   * @param pos Position in line, where paired region to be searched.
   */
  PairMatch* getEnwrappedPairMatch(int lineNo, int pos);

  /**
   * Frees previously allocated PairMatch object.
   * @param pm PairMatch object to free.
   */
  void releasePairMatch(PairMatch* pm);


  /**
   * Return parsed and colored LineRegions of requested line.
   * This method validates current cache state
   * and, if needed, calls Colorer parser to validate modified block of text.
   * Size of reparsed text is choosed according to information
   * about visible text range and modification events.
   * @todo If number of lines, to be reparsed is more, than backParse parameter,
   * then method will return null, until validate() method is called.
   */
  LineRegion* getLineRegions(int lno);

  /**
   * Validates current state of the editor and runs parser, if needed.
   * This method can be called periodically in background thread
   * to make possible background parsing process.
   * @param lno Line number, for which validation is requested.
   *   If this number is in the current visible window range,
   *   the part of text is validated, which is required
   *   for visual repaint.
   *   If this number is equals to -1, all the text is validated.
   *   If this number is not in visible range, optimal partial validation
   *   is used.
   * @param rebuildRegions If true, regions will be recalculated and
   *   repositioned for the specified line number usage. If false,
   *   parser will just start internal cache rebuilding procedure.
   */
  void validate(int lno, bool rebuildRegions);

  /**
   * Tries to do some parsing job while user is doing nothing.
   * @param time integer between 0 and 100, shows an abount of time,
   *             available for this job.
   */
  void idleJob(int time);

  /**
   * Informs BaseEditor object about text modification event.
   * All the text becomes invalid after the specified line.
   * @param topLine Topmost modified line of text.
   */
  void modifyEvent(int topLine);

  /**
   * Informs about single line modification event.
   * Generally, this type of event can be processed much faster
   * because of pre-checking line's changed structure and
   * cancelling further parsing in case of unmodified text structure.
   * @param line Modified line of text.
   * @todo Not used yet! This must include special 'try' parse method.
   */
  void modifyLineEvent(int line);

  /**
   * Informs about changes in visible range of text lines.
   * This information is used to make assumptions about
   * text structure and to make faster parsing.
   * @param wStart Topmost visible line of text.
   * @param wSize  Number of currently visible text lines.
   *               This number must includes all partially visible lines.
   */
  void visibleTextEvent(int wStart, int wSize);

  /**
   * Informs about total lines count change.
   * This must include initial lines number setting.
   */
  void lineCountEvent(int newLineCount);

  /** Basic HRC region - default text (background color) */
  const Region* def_Text;
  /** Basic HRC region - syntax checkable region */
  const Region* def_Syntax;
  /** Basic HRC region - special region */
  const Region* def_Special;
  /** Basic HRC region - Paired region start */
  const Region* def_PairStart;
  /** Basic HRC region - Paired region end */
  const Region* def_PairEnd;

  /** Basic HRC region mapping */
  const RegionDefine* rd_def_Text, *rd_def_HorzCross, *rd_def_VertCross;

  void startParsing(size_t lno);
  void endParsing(size_t lno);
  void clearLine(size_t lno, String* line);
  void addRegion(size_t lno, String* line, int sx, int ex, const Region* region);
  void enterScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme);
  void leaveScheme(size_t lno, String* line, int sx, int ex, const Region* region, const Scheme* scheme);

  bool haveInvalidLine();
  void setMaxBlockSize(int max_block_size);

private:

  FileType* chooseFileTypeCh(const String* fileName, int chooseStr, int chooseLen);

  HRCParser* hrcParser;
  TextParser* textParser;
  ParserFactory* parserFactory;
  LineSource* lineSource;
  RegionMapper* regionMapper;
  LineRegionsSupport* lrSupport;

  FileType* currentFileType;
  std::vector<RegionHandler*> regionHandlers;
  std::vector<EditorListener*> editorListeners;

  int backParse;
  // window area
  int wStart, wSize;
  // line count
  int lineCount;
  // size of line regions
  int lrSize;
  // position of last validLine
  int invalidLine;

 public:
  int getInvalidLine() const;
 private:
  // no lines structure changes, just single line change
  int changedLine;

  bool internalRM;
  bool regionCompact;
  bool breakParse;
  bool validationProcess;

  inline int getLastVisibleLine();
  void remapLRS(bool recreate);
  /**
   * Searches for the paired token and creates PairMatch
   * object with valid initial properties filled.
   */
  PairMatch* getPairMatch(int lineNo, int pos);
};

#endif


