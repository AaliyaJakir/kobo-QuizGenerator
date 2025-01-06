#include <QDialogButtonBox>
#include <QLabel>
#include <QMovie>
#include <QVBoxLayout>
#include <QPushButton>
#include <QRadioButton>
#include <QButtonGroup>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QListWidget>
#include <QProcess>
#include <QSizePolicy>

#include "TemplatePlugin.h"

void TemplatePlugin::showError(const QString& message) 
{
    clearCurrentLayout();
    QVBoxLayout* layout = new QVBoxLayout(&m_dlg);
    
    m_errorLabel = new QLabel(message, &m_dlg);
    m_errorLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"
        "    margin: 10px;"
        "    padding: 5px;"
        "}"
    );
    m_errorLabel->setWordWrap(true);
    m_errorLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_errorLabel);

    m_errorButton = new QPushButton("OK", &m_dlg);
    m_errorButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 28px;"
        "    padding: 10px;"
        "    margin: 10px;"
        "    min-width: 150px;"
        "}"
    );
    layout->addWidget(m_errorButton, 0, Qt::AlignCenter);
    connect(m_errorButton, &QPushButton::clicked, &m_dlg, &QDialog::reject);

    m_dlg.setLayout(layout);
    m_dlg.showDlg();
}

TemplatePlugin::TemplatePlugin() 
    : m_currentIndex(0)
    , m_score(0)
    , m_uiInitialized(false)
{
    // Load quiz questions from JSON file:
    QFile file(QUIZ_QUESTIONS_PATH);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open quiz_questions.json";
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid quiz JSON format!";
        return;
    }

    QJsonArray arr = doc.array();
    for (auto val : arr) {
        QJsonObject obj = val.toObject();
        QuizItem item;
        item.question      = obj["question"].toString();
        
        // Convert JSON array to QStringList manually
        QJsonArray optionsArray = obj["options"].toArray();
        QStringList options;
        for (const QJsonValue &option : optionsArray) {
            options.append(option.toString());
        }
        item.options = options;
        
        item.correctAnswer = obj["correct_answer"].toString();
        m_quizData.push_back(item);
    }
}

void TemplatePlugin::showUi()
{
    // Reset quiz state
    m_currentIndex = 0;
    m_score = 0;
    m_userAnswers.clear();

    // Show book selection UI
    showBookSelection();
}

void TemplatePlugin::updateQuestion()
{
    // Guard against out-of-range
    if (m_currentIndex < 0 || m_currentIndex >= m_quizData.size()) {
        showFinalScore();
        return;
    }

    // Set question text
    m_questionLabel->setText(m_quizData[m_currentIndex].question);

    const QStringList &options = m_quizData[m_currentIndex].options;

    // Clear all radio button selections
    m_buttonGroup->setExclusive(false);
    for (auto btn : m_optionButtons) {
        btn->setChecked(false);
    }
    m_buttonGroup->setExclusive(true);

    // Update the option widgets
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(m_dlg.layout());
    if (!mainLayout) return;

    // Skip the question label (index 0)
    for (int i = 0; i < 4; i++) {
        int layoutIndex = i + 1; // +1 to skip question label
        if (layoutIndex >= mainLayout->count()) break;

        QLayoutItem* item = mainLayout->itemAt(layoutIndex);
        QWidget* optionWidget = item ? item->widget() : nullptr;
        if (!optionWidget) continue;

        // Get the label from the option widget's layout
        if (QHBoxLayout* optLayout = qobject_cast<QHBoxLayout*>(optionWidget->layout())) {
            if (optLayout->count() >= 2) { // Should have radio button and label
                if (QLabel* label = qobject_cast<QLabel*>(optLayout->itemAt(1)->widget())) {
                    if (i < options.size()) {
                        label->setText(options.at(i));
                        optionWidget->show();
                    } else {
                        optionWidget->hide();
                    }
                }
            }
        }
    }
}

void TemplatePlugin::onSubmitClicked()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_quizData.size()) {
        showFinalScore();
        return;
    }

    // Check the selected answer
    int selectedId = m_buttonGroup->checkedId();
    QString chosen;
    if (selectedId >= 0 && selectedId < m_quizData[m_currentIndex].options.size()) {
        chosen = m_quizData[m_currentIndex].options[selectedId];
        // Store user's answer
        m_userAnswers.append(chosen);

        if (chosen == m_quizData[m_currentIndex].correctAnswer) {
            m_score++;
        }
    } else {
        // No selection, store empty string
        m_userAnswers.append(QString());
    }

    // Move to next question or finish
    m_currentIndex++;
    if (m_currentIndex < m_quizData.size()) {
        updateQuestion();
    } else {
        showFinalScore();
    }
}

void TemplatePlugin::showFinalScore()
{
    // Hide all option widgets first
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(m_dlg.layout());
    if (mainLayout) {
        for (int i = 0; i < mainLayout->count(); i++) {
            QLayoutItem* item = mainLayout->itemAt(i);
            if (QWidget* widget = item->widget()) {
                if (widget != m_questionLabel) {  // Don't hide the question label
                    widget->hide();
                }
            }
        }
    }

    m_questionLabel->setText(
        QString("Quiz finished!\nYou scored %1/%2.")
            .arg(m_score)
            .arg(m_quizData.size())
    );

    // Create a new horizontal layout for the final buttons
    m_buttonLayout = new QHBoxLayout();

    // "Review" button
    m_submitButton->setText("Review");
    m_submitButton->show();  // Make sure it's visible
    connect(m_submitButton, &QPushButton::clicked, this, &TemplatePlugin::onReviewClicked);
    m_buttonLayout->addWidget(m_submitButton);

    // "Close" button
    if (!m_secondaryButton) {
        m_secondaryButton = new QPushButton("Close", &m_dlg);
        m_secondaryButton->setStyleSheet(
            "QPushButton {"
            "    font-size: 28px;"
            "    padding: 10px;"
            "    margin: 10px;"
            "    min-width: 150px;"
            "}"
        );
        // Add touch event handling
        m_secondaryButton->setAttribute(Qt::WA_AcceptTouchEvents);
        m_secondaryButton->installEventFilter(&m_dlg);
    }
    
    // Always reconnect the close signal (in case it was disconnected)
    m_secondaryButton->disconnect();
    connect(m_secondaryButton, &QPushButton::clicked, &m_dlg, &QDialog::reject);
    m_buttonLayout->addWidget(m_secondaryButton);

    // Add button layout to main layout
    mainLayout->addLayout(m_buttonLayout);
}

// Add this method to handle touch events for radio buttons
bool TemplatePlugin::eventFilter(QObject *obj, QEvent *event)
{
    // Handle button touch events
    if (QPushButton *button = qobject_cast<QPushButton*>(obj)) {
        if (event->type() == QEvent::TouchBegin ||
            event->type() == QEvent::TouchEnd) {
            
            QMouseEvent *mouseEvent = new QMouseEvent(
                event->type() == QEvent::TouchBegin ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                button->mapFromGlobal(QCursor::pos()),
                Qt::LeftButton,
                Qt::LeftButton,
                Qt::NoModifier
            );
            
            QApplication::postEvent(button, mouseEvent);
            return true;
        }
    }

    // Handle radio button touch events
    if (QRadioButton *radio = qobject_cast<QRadioButton*>(obj)) {
        if (event->type() == QEvent::TouchBegin ||
            event->type() == QEvent::TouchEnd) {
            
            QMouseEvent *mouseEvent = new QMouseEvent(
                event->type() == QEvent::TouchBegin ? QEvent::MouseButtonPress : QEvent::MouseButtonRelease,
                radio->mapFromGlobal(QCursor::pos()),
                Qt::LeftButton,
                Qt::LeftButton,
                Qt::NoModifier
            );
            
            QApplication::postEvent(radio, mouseEvent);
            return true;
        }
    }
    
    // Handle label clicks
    if (QLabel *label = qobject_cast<QLabel*>(obj)) {
        if (event->type() == QEvent::MouseButtonRelease) {
            // Find the associated radio button (sibling in the layout)
            if (QWidget* parentWidget = label->parentWidget()) {
                if (QHBoxLayout* layout = qobject_cast<QHBoxLayout*>(parentWidget->layout())) {
                    if (layout->count() >= 1) {
                        if (QRadioButton* radio = qobject_cast<QRadioButton*>(layout->itemAt(0)->widget())) {
                            radio->setChecked(true);
                            return true;
                        }
                    }
                }
            }
        }
    }

    return QObject::eventFilter(obj, event);
}

void TemplatePlugin::onReviewClicked()
{
    m_currentIndex = 0;

    // Set button text
    m_submitButton->setText(m_quizData.size() > 1 ? "Next" : "Close");
    m_submitButton->disconnect();
    connect(m_submitButton, &QPushButton::clicked, this, &TemplatePlugin::onReviewNextClicked);

    // Make sure the close button is properly connected
    if (m_secondaryButton) {
        m_secondaryButton->disconnect();
        connect(m_secondaryButton, &QPushButton::clicked, &m_dlg, &QDialog::reject);
        m_secondaryButton->show();
    }

    // Show all option widgets
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(m_dlg.layout());
    if (mainLayout) {
        for (int i = 0; i < mainLayout->count(); i++) {
            QLayoutItem* item = mainLayout->itemAt(i);
            if (QWidget* widget = item->widget()) {
                if (widget != m_questionLabel && widget != m_submitButton) {
                    widget->show();
                }
            }
        }
    }

    updateReviewQuestion();
}

void TemplatePlugin::updateReviewQuestion()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_quizData.size()) {
        m_dlg.reject();  // Close dialog
        return;
    }

    // Set question text
    m_questionLabel->setText(m_quizData[m_currentIndex].question);

    const QStringList &options = m_quizData[m_currentIndex].options;
    QString userAnswer = m_userAnswers.value(m_currentIndex);
    QString correctAnswer = m_quizData[m_currentIndex].correctAnswer;

    // Update each option widget
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(m_dlg.layout());
    if (!mainLayout) return;

    // Skip the question label (index 0)
    for (int i = 0; i < 4; i++) {
        int layoutIndex = i + 1; // +1 to skip question label
        if (layoutIndex >= mainLayout->count()) break;

        QLayoutItem* item = mainLayout->itemAt(layoutIndex);
        QWidget* optionWidget = item ? item->widget() : nullptr;
        if (!optionWidget) continue;

        // Get the radio button and label from the option widget's layout
        if (QHBoxLayout* optLayout = qobject_cast<QHBoxLayout*>(optionWidget->layout())) {
            if (optLayout->count() >= 2) {
                QRadioButton* radio = qobject_cast<QRadioButton*>(optLayout->itemAt(0)->widget());
                QLabel* label = qobject_cast<QLabel*>(optLayout->itemAt(1)->widget());
                
                if (radio && label && i < options.size()) {
                    // Set the text
                    label->setText(options.at(i));
                    
                    // Set checked state
                    radio->setChecked(options.at(i) == userAnswer);
                    
                    // Highlight correct/incorrect answers
                    QString color = "black";
                    if (options.at(i) == correctAnswer) {
                        color = "green";
                    } else if (options.at(i) == userAnswer && userAnswer != correctAnswer) {
                        color = "red";
                    }
                    
                    label->setStyleSheet(QString("QLabel { color: %1; }").arg(color));
                    radio->setEnabled(false);
                    optionWidget->show();
                } else {
                    optionWidget->hide();
                }
            }
        }
    }
}

void TemplatePlugin::onReviewNextClicked()
{
    m_currentIndex++;
    if (m_currentIndex < m_quizData.size()) {
        if (m_currentIndex == m_quizData.size() - 1) {
            m_submitButton->setText("Close");
            // Disconnect the "next" connection and connect to close instead
            m_submitButton->disconnect();
            connect(m_submitButton, &QPushButton::clicked, &m_dlg, &QDialog::reject);
            // Hide the secondary close button when we're showing the last question
            if (m_secondaryButton) {
                m_secondaryButton->hide();
            }
        }
        updateReviewQuestion();
    } else {
        m_dlg.reject();
    }
}

void TemplatePlugin::showBookSelection()
{
    clearCurrentLayout();
    QVBoxLayout *layout = new QVBoxLayout(&m_dlg);

    QLabel *label = new QLabel("Select a book to generate quiz:", &m_dlg);
    label->setStyleSheet(
        "QLabel {"
        "    font-size: 38px;"
        "    margin: 10px;"
        "    padding: 5px;"
        "}"
    );
    layout->addWidget(label);

    m_bookListWidget = new QListWidget(&m_dlg);
    m_bookListWidget->setStyleSheet(
        "QListWidget {"
        "    font-size: 32px;"
        "    margin: 10px;"
        "}"
    );

    QFile file(BOOKS_LIST_PATH);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open books.json";
        showError("Unable to open books list file.");
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "Invalid books JSON format!";
        showError("Invalid books list format.");
        return;
    }

    QJsonArray bookArray = doc.object()["books"].toArray();
    QStringList bookTitles;
    for (const QJsonValue &val : bookArray) {
        bookTitles.append(val.toString());
    }

    m_bookListWidget->addItems(bookTitles);
    layout->addWidget(m_bookListWidget);

    // Create button container
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    // Create a select button
    QPushButton *selectButton = new QPushButton("Select", &m_dlg);
    selectButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 28px;"
        "    padding: 10px;"
        "    margin: 10px;"
        "    min-width: 150px;"
        "}"
    );
    buttonLayout->addWidget(selectButton);

    // Create exit button
    QPushButton *exitButton = new QPushButton("Exit", &m_dlg);
    exitButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 28px;"
        "    padding: 10px;"
        "    margin: 10px;"
        "    min-width: 150px;"
        "}"
    );
    buttonLayout->addWidget(exitButton);

    // Add button layout to main layout
    layout->addLayout(buttonLayout);

    connect(selectButton, &QPushButton::clicked, this, &TemplatePlugin::onBookSelected);
    connect(exitButton, &QPushButton::clicked, &m_dlg, &QDialog::reject);

    // Set the layout
    m_dlg.setLayout(layout);

    // Show the dialog
    m_dlg.showDlg();
}

void TemplatePlugin::onBookSelected()
{
    QListWidgetItem *selectedItem = m_bookListWidget->currentItem();
    if (!selectedItem) {
        showError("Please select a book.");
        return;
    }

    QString bookTitle = selectedItem->text();

    // Show loading indicator
    QLabel* loadingLabel = new QLabel("Generating quiz questions...", &m_dlg);
    loadingLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 32px;"
        "    margin: 10px;"
        "    padding: 5px;"
        "}"
    );
    loadingLabel->setAlignment(Qt::AlignCenter);

    // Disable the list and button while loading
    m_bookListWidget->setEnabled(false);
    m_bookListWidget->setStyleSheet(
        "QListWidget {"
        "    font-size: 32px;"
        "    margin: 10px;"
        "    color: gray;"
        "}"
    );

    // Add loading label to the layout
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(m_dlg.layout());
    if (layout) {
        layout->addWidget(loadingLabel);
    }

    // Generate quiz for the selected book
    generateQuizForBook(bookTitle);
}

void TemplatePlugin::generateQuizForBook(const QString &bookTitle)
{
    QString scriptPath = QUIZ_SCRIPT_PATH;
    QStringList arguments;
    arguments << bookTitle;

    QProcess *process = new QProcess(this);
    connect(process, static_cast<void(QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            [=](int exitCode, QProcess::ExitStatus /*exitStatus*/){
        if (exitCode == 0) {
            loadQuizQuestions();
            showQuizUi();
        } else {
            showError("Failed to generate quiz questions. Check your internet connection and try again.");
        }
        process->deleteLater();
    });

    process->start(scriptPath, arguments);
}

void TemplatePlugin::loadQuizQuestions()
{
    m_quizData.clear();

    QFile file(QUIZ_QUESTIONS_PATH);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Unable to open quiz_questions.json";
        showError("Unable to open quiz questions file.");
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        qWarning() << "Invalid quiz JSON format!";
        showError("Invalid quiz questions format.");
        return;
    }

    QJsonArray arr = doc.array();
    for (auto val : arr) {
        QJsonObject obj = val.toObject();
        QuizItem item;
        item.question      = obj["question"].toString();

        // Convert JSON array to QStringList manually
        QJsonArray optionsArray = obj["options"].toArray();
        QStringList options;
        for (const QJsonValue &option : optionsArray) {
            options.append(option.toString());
        }
        item.options = options;

        item.correctAnswer = obj["correct_answer"].toString();
        m_quizData.push_back(item);
    }
}

void TemplatePlugin::showQuizUi()
{
    // Clear existing layout and widgets properly
    clearCurrentLayout();

    // Create new layout
    QVBoxLayout *layout = new QVBoxLayout(&m_dlg);
    layout->setSpacing(10);
    layout->setContentsMargins(20, 20, 20, 20);

    // Create question label
    m_questionLabel = new QLabel(&m_dlg);
    m_questionLabel->setWordWrap(true);
    m_questionLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 55px;"
        "    margin: 10px;"
        "    padding: 5px;"
        "}"
    );
    layout->addWidget(m_questionLabel);

    // Create radio button group
    m_buttonGroup = new QButtonGroup(&m_dlg);
    m_buttonGroup->setExclusive(true);

    m_optionButtons.clear(); // Clear previous buttons if any

    // Create option widgets (radio button + label pairs)
    for (int i = 0; i < 4; i++) {
        QWidget* optionWidget = createOptionWidget(i, "");
        layout->addWidget(optionWidget);
    }

    layout->addSpacing(20);

    // Submit button
    m_submitButton = new QPushButton("Submit", &m_dlg);
    m_submitButton->setStyleSheet(
        "QPushButton {"
        "    font-size: 28px;"
        "    padding: 10px;"
        "    margin: 10px;"
        "    min-width: 150px;"
        "}"
    );
    layout->addWidget(m_submitButton, 0, Qt::AlignCenter);
    connect(m_submitButton, &QPushButton::clicked, this, &TemplatePlugin::onSubmitClicked);

    // Set the new layout (don't delete old one, clearCurrentLayout already did that)
    m_dlg.setLayout(layout);

    // Load the first question
    updateQuestion();

    // Show the dialog
    m_dlg.showDlg();
}

void TemplatePlugin::clearCurrentLayout()
{
    // Disconnect all signals
    if (m_submitButton) {
        m_submitButton->disconnect();
    }
    
    if (QLayout* layout = m_dlg.layout()) {
        // First, handle any nested layouts
        for (int i = 0; i < layout->count(); ++i) {
            QLayoutItem* item = layout->itemAt(i);
            if (QLayout* childLayout = item->layout()) {
                while (QLayoutItem* childItem = childLayout->takeAt(0)) {
                    if (QWidget* widget = childItem->widget()) {
                        widget->hide();
                        widget->deleteLater();
                    }
                    delete childItem;
                }
            }
        }
        
        // Then handle the main layout
        while (QLayoutItem* item = layout->takeAt(0)) {
            if (QWidget* widget = item->widget()) {
                widget->hide();
                widget->deleteLater();
            }
            delete item;
        }
        delete layout;
    }

    // Reset all pointers
    m_questionLabel = nullptr;
    m_buttonGroup = nullptr;
    m_submitButton = nullptr;
    m_secondaryButton = nullptr;
    m_buttonLayout = nullptr;
    m_bookListWidget = nullptr;
    m_optionButtons.clear();
}

// Create a custom widget for each option
QWidget* TemplatePlugin::createOptionWidget(int index, const QString &text)
{
    // A container widget to hold radio button + label
    QWidget *optionWidget = new QWidget(&m_dlg);
    optionWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);

    QHBoxLayout *optionLayout = new QHBoxLayout(optionWidget);
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(10);

    // Create the radio button
    QRadioButton *radio = new QRadioButton(optionWidget);
    radio->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    radio->installEventFilter(this);
    // Add to button group
    m_buttonGroup->addButton(radio, index);
    m_optionButtons.append(radio);

    optionLayout->addWidget(radio);

    // Create a wrapping label for the text
    QLabel *optionLabel = new QLabel(optionWidget);
    optionLabel->setText(text);
    optionLabel->setWordWrap(true); // Will wrap for long text
    optionLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    optionLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 38px;"
        "    margin: 5px;"
        "    padding: 5px;"
        "}"
    );

    // Let the user click the label to toggle the radio button
    optionLabel->installEventFilter(this);

    optionLayout->addWidget(optionLabel);

    return optionWidget;
}