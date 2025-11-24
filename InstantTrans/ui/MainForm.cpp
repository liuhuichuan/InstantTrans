//MainForm.cpp
#include "MainForm.h"


MainForm::MainForm(MessageBus* bus):bus_(bus) {
    flow_.SetUpdateCallback([this](const DisplaySlot& a, const DisplaySlot& b) {
        // UI 回调：在 UI 线程中（PostMessage 已保证）
        this->OnFlowUpdate(a, b);
        });
    recognizer = std::make_shared<SpeechRecognizer>(bus_);
    
    clientid = WebSocketClient::GenerateUUID();
}

MainForm::~MainForm()
{

}

DString MainForm::GetSkinFolder()
{
    return _T("my_duilib_app");
}

DString MainForm::GetSkinFile()
{
    return _T("MyDuilibForm.xml");
}

void MainForm::OnInitWindow()
{
    BaseClass::OnInitWindow();

    bus_->SetUIWindow(static_cast<HWND>(this->GetWindowHandle()));

    //窗口初始化完成，可以进行本Form的初始化
    m_pLabelActiveRecog = dynamic_cast<ui::Label*>(FindControl(L"origin_text1"));
    m_pLabelActiveTrans = dynamic_cast<ui::Label*>(FindControl(L"trans_text1"));
    m_pLabelNextRecog = dynamic_cast<ui::Label*>(FindControl(L"origin_text2"));
    m_pLabelNextTrans = dynamic_cast<ui::Label*>(FindControl(L"trans_text2"));

    m_pBtnAction = dynamic_cast<ui::Button*>(FindControl(L"actionbtn"));
    m_pBtnAction->AttachClick(std::bind(&MainForm::onSwitchState, this, std::placeholders::_1));
    
    //SetTimer(static_cast<HWND>(this->GetWindowHandle()), 1, 1000, nullptr);
}

void MainForm::OnPreCloseWindow()
{
    if (m_runningstate)
    {
        recognizer->Stop();
    }

    __super::OnPreCloseWindow();
}

LRESULT MainForm::OnWindowMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, bool& bHandled)
{
    if (uMsg == WM_APP_RECOG) {
        auto p = reinterpret_cast<RecognitionMessage*>(wParam);
        OnRecognitionMessage(*p);
        delete p;
        return 0;
    }
    else if (uMsg == WM_APP_TRANS) {
        auto p = reinterpret_cast<TranslationMessage*>(wParam);
        OnTranslationMessage(*p);
        delete p;
        return 0;
    }
    else if (uMsg == WM_TIMER)
    {
        OnTimerImpl(wParam, lParam);
        return 0;
    }
    return __super::OnWindowMessage(uMsg, wParam, lParam, bHandled);
}

void MainForm::OnTimerImpl(WPARAM wParam, LPARAM lParam)
{
    if (wParam == 1)
    {
        Tick();
    }
}

void MainForm::OnRecognitionMessage(const RecognitionMessage& msg) {
    // 如果 msg.is_final == true -> 启动翻译请求线程（避免阻塞识别线程）
    if (!msg.is_final) {
        // 片段更新 -> 交给 flow 的 next slot（bottom）
        flow_.OnRecognitionFragment(msg.recog_text);
    }
    else {
        // final：发起翻译异步请求
        std::string recog = msg.recog_text;

        std::string trans;
        bool ok = WebSocketClient::SendTranslateAndReceive(
            ws,
            clientid,
            "en",
            "zh",
            recog,
            trans
        );
        if (trans.empty())
            trans = "Hello InstantTrans";
        TranslationMessage tmsg;
        tmsg.recog_text = recog;
        tmsg.trans_text = trans;
        // 把翻译结果发到 UI（本对象的 bus_）
        if (bus_) bus_->PostTranslation(tmsg);
    }
}

void MainForm::OnTranslationMessage(const TranslationMessage& msg) {
    // 翻译到达 UI（已经在 UI 线程）
    // 告知流控器：把翻译放到 next slot（或直接让 flow 决定）
    flow_.OnTranslationReady(msg.recog_text, msg.trans_text);
}

void MainForm::OnFlowUpdate(const DisplaySlot& active, const DisplaySlot& next) {
    //  把 active、next 的 recog/trans 写进 DuiLib 的 Label 控件
     m_pLabelActiveRecog->SetText(ui::StringConvert::UTF8ToWString(active.recog));
     m_pLabelActiveTrans->SetText(ui::StringConvert::UTF8ToWString(active.trans));
     m_pLabelNextRecog->SetText(ui::StringConvert::UTF8ToWString(next.recog));
     m_pLabelNextTrans->SetText(ui::StringConvert::UTF8ToWString(next.trans));
    // TODO:
    // 如果发生了滚动（active 替换），做动画或即时切换
}

void MainForm::Tick() {
    flow_.Tick();
}

MessageBus* MainForm::GetBus() { return bus_; }

bool MainForm::onSwitchState(const ui::EventArgs& args)
{
    if (!m_runningstate)
    {
        m_runningstate = true;

        ws = WebSocketClient::WS_Connect("ws://127.0.0.1:8080/ws");
        m_pBtnAction->SetText(L"停止");

        recognizer->Start();
    }
    else
    {
        m_runningstate = false;

        WebSocketClient::WS_Close(ws);

        m_pBtnAction->SetText(L"启动");

        recognizer->Stop();
    }
    return true;
}