/*
 * Surge XT - a free and open source hybrid synthesizer,
 * built by Surge Synth Team
 *
 * Learn more at https://surge-synthesizer.github.io/
 *
 * Copyright 2018-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * Surge XT is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 *
 * Surge was a commercial product from 2004-2018, copyright and ownership
 * held by Claes Johanson at Vember Audio during that period.
 * Claes made Surge open source in September 2018.
 *
 * All source for Surge XT is available at
 * https://github.com/surge-synthesizer/surge
 */

/*
 * This file contains all the various LUA editors as specialized
 * classes and shared function base classes.
 */
#ifndef SURGE_SRC_SURGE_XT_GUI_OVERLAYS_LUAEDITORS_H
#define SURGE_SRC_SURGE_XT_GUI_OVERLAYS_LUAEDITORS_H

#include <juce_gui_extra/juce_gui_extra.h>

#include "WavetableScriptEvaluator.h"

#include "SurgeStorage.h"
#include "SkinSupport.h"

#include "OverlayComponent.h"
#include "RefreshableOverlay.h"

class SurgeGUIEditor;

namespace Surge
{
namespace Overlays
{

class CodeEditorSearch : public juce::Component,
                         public juce::TextEditor::Listener,
                         public juce::KeyListener,
                         public Surge::GUI::SkinConsumingComponent
{
  private:
    virtual void setHighlightColors();
    virtual void removeHighlightColors();
    juce::CodeEditorComponent *ed;
    bool active = false;
    int result[128];
    int resultCurrent;
    int resultTotal;
    bool saveCaretStartPositionLock;
    Surge::GUI::Skin::ptr_t currentSkin;

    std::unique_ptr<juce::TextEditor> textfield;
    std::unique_ptr<juce::Label> labelResult;

    juce::CodeDocument::Position startCaretPosition;

    virtual void search();

  public:
    virtual bool isActive();
    virtual void show();
    virtual void hide();
    virtual void resize();
    virtual void textEditorTextChanged(juce::TextEditor &textEditor) override;
    virtual void mouseDown(const juce::MouseEvent &event) override;
    virtual void focusLost(FocusChangeType) override;

    CodeEditorSearch(juce::CodeEditorComponent &editor, Surge::GUI::Skin::ptr_t);
    virtual void textEditorEscapeKeyPressed(juce::TextEditor &) override;
    virtual void textEditorReturnKeyPressed(juce::TextEditor &) override;
    virtual bool keyPressed(const juce::KeyPress &key,
                            juce::Component *originatingComponent) override;
    virtual void saveCaretStartPosition(bool onlyReadCaretPosition);
    virtual void showResult(int increase, bool moveCaret);
    virtual void paint(juce::Graphics &g) override;
};

class SurgeCodeEditorComponent : public juce::CodeEditorComponent
{
  public:
    virtual void handleEscapeKey() override;
    virtual void handleReturnKey() override;
    virtual void caretPositionMoved() override;

    virtual void setSearch(CodeEditorSearch &s);
    SurgeCodeEditorComponent(juce::CodeDocument &d, juce::CodeTokeniser *t);

  private:
    CodeEditorSearch *search = nullptr;
};

/*
 * This is a base class that provides you an apply button, an editor, a document
 * a tokenizer, etc... which you need to layout with yoru other components by
 * having a superclass constructor and implementing resized; and where you have
 * to handle the apply condition with applyCode()
 */
class CodeEditorContainerWithApply : public OverlayComponent,
                                     public juce::CodeDocument::Listener,
                                     public juce::Button::Listener,
                                     public juce::KeyListener,
                                     public Surge::GUI::SkinConsumingComponent
{
  public:
    CodeEditorContainerWithApply(SurgeGUIEditor *ed, SurgeStorage *s, Surge::GUI::Skin::ptr_t sk,
                                 bool addComponents = false);
    std::unique_ptr<juce::CodeDocument> mainDocument;
    std::unique_ptr<SurgeCodeEditorComponent> mainEditor;
    std::unique_ptr<juce::Button> applyButton;
    std::unique_ptr<juce::LuaTokeniser> tokenizer;
    std::unique_ptr<CodeEditorSearch> search;

    void buttonClicked(juce::Button *button) override;
    void codeDocumentTextDeleted(int startIndex, int endIndex) override;
    void codeDocumentTextInserted(const juce::String &newText, int insertIndex) override;
    void removeTrailingWhitespaceFromDocument();
    bool autoCompleteStringDeclaration(juce::String str);
    bool keyPressed(const juce::KeyPress &key, Component *originatingComponent) override;

    virtual void setApplyEnabled(bool) {}

    void paint(juce::Graphics &g) override;
    SurgeGUIEditor *editor;
    SurgeStorage *storage;

    virtual void onSkinChanged() override;

    virtual void applyCode() = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditorContainerWithApply);
};

struct ExpandingFormulaDebugger;
struct FormulaControlArea;

struct FormulaModulatorEditor : public CodeEditorContainerWithApply, public RefreshableOverlay
{
    FormulaModulatorEditor(SurgeGUIEditor *ed, SurgeStorage *s, LFOStorage *lfos,
                           FormulaModulatorStorage *fs, int lfoid, int scene,
                           Surge::GUI::Skin::ptr_t sk);
    ~FormulaModulatorEditor();

    void resized() override;
    void onSkinChanged() override;
    void applyCode() override;
    void forceRefresh() override;
    void setApplyEnabled(bool b) override;
    void showModulatorCode();
    void showPreludeCode();
    void escapeKeyPressed();

    void updateDebuggerIfNeeded();

    std::unique_ptr<juce::CodeDocument> preludeDocument;
    std::unique_ptr<juce::CodeEditorComponent> preludeDisplay;
    std::unique_ptr<FormulaControlArea> controlArea;

    std::unique_ptr<ExpandingFormulaDebugger> debugPanel;

    LFOStorage *lfos{nullptr};
    FormulaModulatorStorage *formulastorage{nullptr};
    SurgeGUIEditor *editor{nullptr};
    int lfo_id, scene;
    int32_t updateDebuggerCounter{0};

    DAWExtraStateStorage::EditorState::FormulaEditState &getEditState();

    bool shouldRepaintOnParamChange(const SurgePatch &patch, Parameter *p) override
    {
        return false;
    }

    std::optional<std::pair<std::string, std::string>> getPreCloseChickenBoxMessage() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FormulaModulatorEditor);
};

struct WavetablePreviewComponent;
struct WavetableScriptControlArea;

struct WavetableScriptEditor : public CodeEditorContainerWithApply, public RefreshableOverlay
{
    WavetableScriptEditor(SurgeGUIEditor *ed, SurgeStorage *s, OscillatorStorage *os, int oscid,
                          int scene, Surge::GUI::Skin::ptr_t sk);
    ~WavetableScriptEditor();

    void resized() override;
    void onSkinChanged() override;
    void applyCode() override;
    void forceRefresh() override;
    void setApplyEnabled(bool b) override;
    void showModulatorCode();
    void showPreludeCode();
    void escapeKeyPressed();

    void generateWavetable();
    void rerenderFromUIState();
    void setCurrentFrame(int value);

    int lastRes{-1}, lastFrames{-1}, lastFrame{-1}, lastRm{-1};

    void setupEvaluator();

    std::unique_ptr<Surge::WavetableScript::LuaWTEvaluator> evaluator;
    std::unique_ptr<juce::CodeDocument> preludeDocument;
    std::unique_ptr<juce::CodeEditorComponent> preludeDisplay;
    std::unique_ptr<WavetableScriptControlArea> controlArea;

    std::unique_ptr<WavetablePreviewComponent> rendererComponent;

    OscillatorStorage *osc;
    SurgeGUIEditor *editor{nullptr};
    int osc_id, scene;

    DAWExtraStateStorage::EditorState::WavetableScriptEditState &getEditState();

    bool shouldRepaintOnParamChange(const SurgePatch &patch, Parameter *p) override
    {
        return false;
    }

    std::optional<std::pair<std::string, std::string>> getPreCloseChickenBoxMessage() override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WavetableScriptEditor);
};

} // namespace Overlays
} // namespace Surge

#endif // SURGE_XT_LUAEDITORS_H
