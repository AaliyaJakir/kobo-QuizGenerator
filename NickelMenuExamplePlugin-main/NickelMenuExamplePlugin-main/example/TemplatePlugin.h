#ifndef TEMPLATE_PLUGIN_H
#define TEMPLATE_PLUGIN_H

#include <QObject>
#include "NPDialog.h"
#include "../NPGuiInterface.h"
#include <QString>
#include <QStringList>
#include <QLabel>
#include <QButtonGroup>
#include <QPushButton>
#include <QRadioButton>
#include <QHBoxLayout>
#include <QListWidget>
#include <QProcess>

// Script paths
const QString QUIZ_SCRIPT_PATH = "/mnt/onboard/.adds/quiz/generateQuiz.sh";
const QString QUIZ_QUESTIONS_PATH = "/mnt/onboard/.adds/quiz/quiz_questions.json";
const QString BOOKS_LIST_PATH = "/mnt/onboard/.adds/quiz/books.json";

struct QuizItem {
    QString question;
    QStringList options;
    QString correctAnswer;
};

class TemplatePlugin : public QObject, public NPGuiInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID NPGuiInterfaceIID FILE "TemplatePlugin.json")
    Q_INTERFACES(NPGuiInterface)

    public:
        TemplatePlugin();
        ~TemplatePlugin() {
            clearCurrentLayout();
        }
        void showUi();

    protected:
        bool eventFilter(QObject *obj, QEvent *event) override;

    private:
        void updateQuestion();
        void onSubmitClicked();
        void showFinalScore();
        void showError(const QString& message);

        // New methods for book selection
        void showBookSelection();
        void onBookSelected();
        void generateQuizForBook(const QString &bookTitle);
        void loadQuizQuestions();
        void showQuizUi();
        void handleBookScrollUp();
        void handleBookScrollDown();

        // New functions for review
        void onReviewClicked();
        void updateReviewQuestion();
        void onReviewNextClicked();

        NPDialog m_dlg;
        QLabel* m_questionLabel = nullptr;
        QButtonGroup* m_buttonGroup = nullptr;
        QPushButton* m_submitButton = nullptr;
        QPushButton* m_secondaryButton = nullptr; // For the "Close" button
        QList<QuizItem> m_quizData;
        int m_currentIndex;
        int m_score;
        bool m_uiInitialized = false; // Track if UI is initialized
        QList<QRadioButton*> m_optionButtons; // Keep references to radio buttons
        QStringList m_userAnswers;  // To store user's answers

        QHBoxLayout* m_buttonLayout = nullptr; // To hold the buttons at the end
        QListWidget* m_bookListWidget = nullptr;
        QPushButton* m_bookScrollUpButton = nullptr;
        QPushButton* m_bookScrollDownButton = nullptr;

        // Error dialog widgets
        QWidget* m_errorWidget = nullptr;
        QLabel* m_errorLabel = nullptr;
        QPushButton* m_errorButton = nullptr;

        // Helper methods
        void clearCurrentLayout();
        QWidget* createOptionWidget(int index, const QString &text);
};

#endif // TEMPLATE_PLUGIN_H