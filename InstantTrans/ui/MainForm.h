//MainForm.h
#ifndef EXAMPLES_MAIN_FORM_H_
#define EXAMPLES_MAIN_FORM_H_

// duilib
#include "duilib/duilib.h"

#include "controller/FlowController.h"
#include "types/types.h"
#include "core/ipc/MessageBus.h"
#include "core/recoginize/SpeechRecognize.h"
#include "core/translate/WSHelper.h"

/** 应用程序的主窗口实现
*/
class MainForm : public ui::WindowImplBase
{
    typedef ui::WindowImplBase BaseClass;
public:
    MainForm(MessageBus* bus);
    virtual ~MainForm() override;

    /**  创建窗口时被调用，由子类实现用以获取窗口皮肤目录
    * @return 子类需实现并返回窗口皮肤目录
    */
    virtual DString GetSkinFolder() override;

    /**  创建窗口时被调用，由子类实现用以获取窗口皮肤 XML 描述文件
    * @return 子类需实现并返回窗口皮肤 XML 描述文件
    *         返回的内容，可以是XML文件内容（以字符'<'为开始的字符串），
    *         或者是文件路径（不是以'<'字符开始的字符串），文件要在GetSkinFolder()路径中能够找到
    */
    virtual DString GetSkinFile() override;

    /** 当窗口创建完成以后调用此函数，供子类中做一些初始化的工作
    */
    virtual void OnInitWindow() override;

    virtual void OnTimerImpl(WPARAM wParam, LPARAM lParam);

    virtual LRESULT OnWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled) override;

    virtual void OnPreCloseWindow() override;
public:
    void OnRecognitionMessage(const RecognitionMessage& msg);

    void OnTranslationMessage(const TranslationMessage& msg);

    /** UI更新回调
    */
    void OnFlowUpdate(const DisplaySlot& active, const DisplaySlot& next);

    void Tick();

    MessageBus* GetBus();

    bool onSwitchState(const ui::EventArgs& args);

private:
    // 控件指针
     ui::Label* m_pLabelActiveRecog = nullptr;
     ui::Label* m_pLabelActiveTrans = nullptr;
     ui::Label* m_pLabelNextRecog = nullptr;
     ui::Label* m_pLabelNextTrans = nullptr;

     ui::Button* m_pBtnAction = nullptr;

     FlowController flow_;
     MessageBus* bus_;
     std::shared_ptr<SpeechRecognizer> recognizer;

     bool m_runningstate = false;

     CURL* ws = NULL;
     std::string clientid = "";
};

#endif //EXAMPLES_MAIN_FORM_H_
