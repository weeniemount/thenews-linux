#include <glib.h>
#include <libnotify/notify.h>
#include <iostream>

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QStackedWidget>
#include "ui_page1.h"
#include "ui_page2.h"
#include <QLabel>
#include <QFontDatabase>
#include <QFile>
#include <QImage>
#include <QBuffer>
#include <QPainter>
#include <QStyleOption>
#include <QStandardPaths>
#include <QDir>
#include <QTextStream>
#include <QTimer>
#include <QSlider>
#include <QHBoxLayout>

class SkewedButton : public QPushButton {
public:
    SkewedButton(QWidget *parent = nullptr) : QPushButton(parent) {
        setAttribute(Qt::WA_TranslucentBackground, false);
        setAttribute(Qt::WA_OpaquePaintEvent, false);
    }
    
    void setSkewTransform(double skewX, double skewY, double translateX = 0, double translateY = 0, double rotation = 0) {
        m_skewX = skewX;
        m_skewY = skewY;
        m_translateX = translateX;
        m_translateY = translateY;
        m_rotation = rotation;
        
        QTransform transform;
        QPointF center(width() / 2.0, height() / 2.0);
        transform.translate(center.x(), center.y());
        if (m_rotation != 0) {
            transform.rotate(m_rotation);
        }
        double shearX = qTan(qDegreesToRadians(m_skewX));
        double shearY = qTan(qDegreesToRadians(m_skewY));
        transform.shear(shearX, shearY);
        transform.translate(-center.x(), -center.y());
        
        QRectF transformedRect = transform.mapRect(QRectF(rect()));
        
        QRect originalGeom = geometry();
        int extraLeft = qMax(0.0, -transformedRect.left());
        int extraTop = qMax(0.0, -transformedRect.top());
        int extraRight = qMax(0.0, transformedRect.right() - rect().width());
        int extraBottom = qMax(0.0, transformedRect.bottom() - rect().height());
        
        m_originalRect = QRect(extraLeft, extraTop, originalGeom.width(), originalGeom.height());
        
        setGeometry(
            originalGeom.x() - extraLeft,
            originalGeom.y() - extraTop,
            originalGeom.width() + extraLeft + extraRight,
            originalGeom.height() + extraTop + extraBottom
        );
        
        update();
    }
    
protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        QTransform transform;
        
        QRectF drawRect = m_originalRect.isNull() ? rect() : m_originalRect;
        QPointF center(drawRect.center());
        
        transform.translate(center.x(), center.y());
        
        if (m_rotation != 0) {
            transform.rotate(m_rotation);
        }
        
        double shearX = qTan(qDegreesToRadians(m_skewX));
        double shearY = qTan(qDegreesToRadians(m_skewY));
        transform.shear(shearX, shearY);
        
        transform.translate(-center.x(), -center.y());
        
        painter.setTransform(transform);
        
        QColor bgColor = palette().color(QPalette::Active, QPalette::Button);
        QColor borderColor = Qt::black;
        
        if (isDown()) {
            bgColor = bgColor.darker(120);
        } else if (underMouse()) {
            bgColor = bgColor.lighter(110);
        }
        
        painter.fillRect(drawRect.toRect(), bgColor);
        
        painter.setPen(QPen(borderColor, 1));
        painter.drawRect(drawRect.toRect().adjusted(0, 0, -1, -1));
        
        painter.setPen(palette().color(QPalette::ButtonText));
        QFont font = this->font();
        painter.setFont(font);
        painter.drawText(drawRect.toRect(), Qt::AlignCenter, text());
    }
    
    void enterEvent(QEnterEvent *event) override {
        update();
        QPushButton::enterEvent(event);
    }
    
    void leaveEvent(QEvent *event) override {
        update();
        QPushButton::leaveEvent(event);
    }
    
    void mousePressEvent(QMouseEvent *event) override {
        update();
        QPushButton::mousePressEvent(event);
    }
    
    void mouseReleaseEvent(QMouseEvent *event) override {
        update();
        QPushButton::mouseReleaseEvent(event);
    }
    
private:
    double m_skewX = 0;
    double m_skewY = 0;
    double m_translateX = 0;
    double m_translateY = 0;
    double m_rotation = 0;
    QRect m_originalRect;
};

void createHDesktopFile() {
    // free downloadable h
    QString applicationsPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    
    QDir dir(applicationsPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString iconPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons/h.gif";
    QDir iconDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/icons");
    if (!iconDir.exists()) {
        iconDir.mkpath(".");
    }
    
    QFile::copy(":/assets/h.gif", iconPath);
    QFile::setPermissions(iconPath, QFile::ReadOwner | QFile::WriteOwner | QFile::ReadGroup | QFile::ReadOther);
    
    QString desktopFilePath = applicationsPath + "/h.desktop";
    QFile desktopFile(desktopFilePath);
    
    if (desktopFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&desktopFile);
        out << "[Desktop Entry]\n";
        out << "Type=Application\n";
        out << "Name=h\n";
        out << "Comment=h\n";
        out << "Icon=" << iconPath << "\n";
        out << "Exec=/bin/true\n";
        out << "Terminal=false\n";
        out << "Categories=Utility;\n";
        
        desktopFile.close();
        
        QFile::setPermissions(desktopFilePath, 
            QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner |
            QFile::ReadGroup | QFile::ExeGroup |
            QFile::ReadOther | QFile::ExeOther);
        
        std::cout << "h deployed at: " << desktopFilePath.toStdString() << "\n";
    } else {
        std::cerr << "failed to h...\n";
    }
}

void setNotificationImageFromResource(NotifyNotification *n, const QString &resourcePath) {
    QImage image(resourcePath);
    if (image.isNull()) {
        std::cerr << "the image is corrupted or not there idfk\n";
        return;
    }
    
    image = image.convertToFormat(QImage::Format_RGBA8888);
    
    int width = image.width();
    int height = image.height();
    int rowstride = image.bytesPerLine();
    bool hasAlpha = true;
    int bitsPerSample = 8;
    int channels = 4;
    
    GBytes *bytes = g_bytes_new(image.constBits(), image.sizeInBytes());
    
    GVariant *imageData = g_variant_new("(iiibii@ay)",
        width, height, rowstride, hasAlpha, bitsPerSample, channels,
        g_variant_new_from_bytes(G_VARIANT_TYPE_BYTESTRING, bytes, TRUE)
    );
    
    notify_notification_set_hint(n, "image-data", imageData);
    g_bytes_unref(bytes);
}

// no but seriously why
static void doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing(NotifyNotification *notification, char *action, gpointer user_data) {
    std::cout << "ok\n";
}

void sendNotification(const std::string &notificationType) {
    // unfortunatley libnotify isnt as advanced like windows xaml com object whatever notifications so we have to remove some stuff to make it still work

    static bool initialized = false;
    if (!initialized) {
        if (!notify_init("the news")) {
            std::cerr << "libnotify is not notifying\n";
            return;
        }
        initialized = true;
    }

    std::string title;
    std::string message;
    NotifyNotification *n = nullptr;

    if (notificationType == "someoneDied") {
        title = "BREAKING NEWS!!!";
        message = "Someone just died! Who? We don't know.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
    } else if (notificationType == "donate") {
        title = "we need your money";
        message = "donate to \"the news\"\n\n";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        notify_notification_add_action(n, "donate_100k", "100k dollars", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "donate_1k", "1k dollars", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "donate_1", "1 dollar", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "serversDying") {
        title = "please donate us money";
        message = "our in house servers ae dying of money :(\n\n";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
        
        notify_notification_add_action(n, "donate_100k", "100k dollars", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "donate_1k", "1k dollars", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "donate_1", "1 dollar", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "deleteSystem32") {
        title = "welp";
        message = "since you didn't donate to the news...\ndeleting system32...\n21/15,245 files";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        notify_notification_set_hint(n, "value", g_variant_new_int32(50));
        notify_notification_set_hint(n, "synchronous", g_variant_new_string("system32-delete"));

        notify_notification_add_action(n, "cope", "cope ¯\\_(\\ツ)_/¯", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "donate_late", "donate before its late", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
    } else if (notificationType == "incomingCall") {
        title = "John Phone";
        message = "Incoming Call - Satellite";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/johnphone.jpg");
        notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
        notify_notification_set_hint(n, "category", g_variant_new_string("im.received"));

        notify_notification_add_action(n, "answer", "Answer", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "earthOnFire") {
        title = "BREAKING NEWS! the earth is on fire lmfao";
        message = "weather forecast:\n\n"
                  "Mon: ☀️ 63° / 42°\n"
                  "Tue: ☀️ 78° / 60°\n"
                  "Wed: ☀️ 96° / 76°\n"
                  "Thu: ☀️ 132° / 89°\n"
                  "Fri: ☀️ 244° / 120°";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
    } else if (notificationType == "friendRequest") {
        title = "John Phone sent you a friend request";
        message = "i want Sponsorships.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/johnphone.jpg");
        notify_notification_add_action(n, "accept", "Accept", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "decline", "Decline", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "websiteRedesign") {
        title = "we redesigned our website";
        message = "enjoy it and leave feed back";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/redesign.png");
        
        notify_notification_add_action(n, "good", "good", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "horrid", "horrid", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "roadblocks") {
        title = "BREAKING NEWS!!!";
        message = "California man posts TikTok of him riding in his golf cart rambling on about 'roadblocks' on the beach, goes crazy fucking viral.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/roadblocks.gif");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "linkerTragedy") {
        title = "BREAKING NEWS!!!";
        message = "Discord user @linker.sh, from the server 'Face's attic', goes all in on black, loses it all in 1 night - tragedy unfolds.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/linker.sh.png");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "mazeGambled") {
        title = "BREAKING NEWS!!!!!!!!!!!!!!!!!!!!!";
        message = "MAZE CONCENTRATED ON GAMBLING SO HARD THEY GOT $-1 IN RETURN???";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/maze.png");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "femboyLabs") {
        title = "BREAKING NEWS!!!";
        message = "femboyLabs has rebranded again!";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/astolfo.jpg");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "bussinIndustries") {
        title = "BREAKING NEWS!!!";
        message = "Bussin Industries shares cryptic note on staff channels.\n\nSource: X";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/bussin_industries.png");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "hTile") {
        createHDesktopFile();
        
        title = "h";
        message = "check your start menu and enjoy your free h";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/h.gif");
        
    } else if (notificationType == "baseballEmoji") {
        title = "⚾️ Baseball on Discord?! 🤯";
        message = "Fr fr, a baseball emoji just dropped on Discord. Icl, ts kinda mogging ngl. 🤣";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/whateverthisis.png");
        
        notify_notification_add_action(n, "read_more", "Read More", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "discard", "discard", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "jonathanPork") {
        title = "John Pork";
        message = "Incoming Call - Satellite";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/johnpork.jpg");
        notify_notification_set_urgency(n, NOTIFY_URGENCY_CRITICAL);
        notify_notification_set_hint(n, "category", g_variant_new_string("im.received"));

        notify_notification_add_action(n, "answer", "Answer", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "linkerAgain") {
        title = "Text message from +1 248-434-5508";
        message = "We've successfully assassinated the attacker. Thank you for contacting Valve Support.\n\nLinker's Samsung Galaxy";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        notify_notification_add_action(n, "gamble", "Gamble it all away", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "hGif") {
        title = "h";
        message = "h";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/h.gif");
        
    } else if (notificationType == "findMeOnline") {
        title = "John Phone";
        message = "Find me online";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/itsme...johnphone.jpg");
        
        notify_notification_add_action(n, "send", "Send", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "googServices") {
        title = "the news needs Google Play Services";
        message = "the news uses Google Play Services to provide you a better experience. Install it. Right now. I don't care that you are using a desktop OS. Install it.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/googleplayservices.png");
        
    } else if (notificationType == "flash") {
        title = "BREAKING NEWS!!!";
        message = "To view this notification, install Adobe® Flash Player™";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/flashplayer.png");
        
    } else if (notificationType == "mcafee") {
        title = "BREAKING NEWS!!!";
        message = "Your McAfee™ subscription plan has expired. Please select a new one below.\n\nEssential - $119.99\nMcAfee+™ Premium Individual - $149.99\nMcAfee+™ Advanced Individual - $199.99";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/mcafee.png");
        
        notify_notification_add_action(n, "subscribe", "yeah this gud!", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        notify_notification_add_action(n, "cancel", "no never cancel it rn", doAbsolutleyNothingBecauseWhyDoINeedThisAsAFunctionCantIJustDoNothing, nullptr, nullptr);
        
    } else if (notificationType == "noskid") {
        title = "BREAKING NEWS!!!";
        message = "To view this notification, upload a NoSkid certificate.";
        
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
        
        setNotificationImageFromResource(n, ":/assets/noskid.png");
        
    } else {
        title = "no notification :(";
        message = "notification doesnt exist somehow what did i call to get this...?";
        n = notify_notification_new(title.c_str(), message.c_str(), nullptr);
    }

    GError *error = nullptr;
    if (!notify_notification_show(n, &error)) {
        if (error) {
            std::cerr << "error notifying the notification smh: " << error->message << "\n";
            g_error_free(error);
        }
    }cd

    g_object_unref(G_OBJECT(n));
}

auto addConsistentStyle = [](QPushButton *btn) {
    QPalette palette = btn->palette();
    QColor baseColor = palette.color(QPalette::Button);
    baseColor.setAlpha(100);
    
    QColor hoverColor = baseColor.lighter(110);
    hoverColor.setAlpha(100);
    
    QColor pressColor = baseColor.darker(120);
    pressColor.setAlpha(100);
    
    btn->setStyleSheet(
        QString("QPushButton { "
                "background-color: rgba(%1, %2, %3, %4); "
                "color: black; "
                "border: 1px solid black; "
                "} "
                "QPushButton:hover { "
                "background-color: rgba(%5, %6, %7, %8); "
                "} "
                "QPushButton:pressed { "
                "background-color: rgba(%9, %10, %11, %12); "
                "}")
            .arg(baseColor.red()).arg(baseColor.green()).arg(baseColor.blue()).arg(baseColor.alpha())
            .arg(hoverColor.red()).arg(hoverColor.green()).arg(hoverColor.blue()).arg(hoverColor.alpha())
            .arg(pressColor.red()).arg(pressColor.green()).arg(pressColor.blue()).arg(pressColor.alpha())
    );
};


int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    int fontId = QFontDatabase::addApplicationFont(":/assets/nimbusroman.otf");
    QString family;
    if (fontId != -1) {
        family = QFontDatabase::applicationFontFamilies(fontId).at(0);
    }

    QMainWindow window;
    window.setWindowIcon(QIcon(":/assets/thenews.png"));
    window.setWindowTitle("the news");
    
    QStackedWidget *stackedWidget = new QStackedWidget();
    
    // page 1!
    QMainWindow *page1Window = new QMainWindow();
    Ui_MainWindow ui;
    ui.setupUi(page1Window);
    stackedWidget->addWidget(page1Window);
    
    // ok now we stack page 2 on it
    QMainWindow *page2Window = new QMainWindow();
    Ui_Page2Window page2Ui;
    page2Ui.setupUi(page2Window);
    stackedWidget->addWidget(page2Window);
    
    window.setCentralWidget(stackedWidget);

    auto replaceWithSkewed = [](QPushButton *original, double skewX, double skewY, double translateX, double translateY, double rotation) -> SkewedButton* {
        SkewedButton *skewed = new SkewedButton(original->parentWidget());
        skewed->setGeometry(original->geometry());
        skewed->setText(original->text());
        skewed->setPalette(original->palette());
        skewed->setSkewTransform(skewX, skewY, translateX, translateY, rotation);
        
        original->hide();
        return skewed;
    };

    auto skewedDonation = replaceWithSkewed(ui.donation, 0, 29.612, -11.22, 18.454, 31.3);
    QObject::connect(skewedDonation, &QPushButton::clicked, []() {
        sendNotification("donate");
    });

    auto skewedSysem32 = replaceWithSkewed(ui.sysem32, 0, 13.325, 0, 8.763, 0);
    QObject::connect(skewedSysem32, &QPushButton::clicked, []() {
        sendNotification("deleteSystem32");
    });

    auto skewedJohnPhone = replaceWithSkewed(ui.johnPhone, -30.472, 0, -52.662, 0, 0);
    QObject::connect(skewedJohnPhone, &QPushButton::clicked, []() {
        sendNotification("incomingCall");
    });

    auto skewedJohnPhoneFQ = replaceWithSkewed(ui.johnPhoneFQ, -16.861, 0, 5.796, 0, 0);
    QObject::connect(skewedJohnPhoneFQ, &QPushButton::clicked, []() {
        sendNotification("friendRequest");
    });

    auto skewedRoadblocks = replaceWithSkewed(ui.roadblocks, 0, 8.005, 0, 4.359, 0);
    QObject::connect(skewedRoadblocks, &QPushButton::clicked, []() {
        sendNotification("roadblocks");
    });

    auto skewedMazeGambled = replaceWithSkewed(page2Ui.mazeGambled, -45, 0, 16, 0, 0);
    QObject::connect(skewedMazeGambled, &QPushButton::clicked, []() {
        sendNotification("mazeGambled");
    });

    auto skewedFemboyLabs = replaceWithSkewed(page2Ui.femboyLabs, 31.997, 0, -9.375, 2.713, -16.137);
    QObject::connect(skewedFemboyLabs, &QPushButton::clicked, []() {
        sendNotification("femboyLabs");
    });

    auto skewedBaseballEmoji = replaceWithSkewed(page2Ui.baseballEmoji, -61.557, 0, -6.784, 0, 0);
    QObject::connect(skewedBaseballEmoji, &QPushButton::clicked, []() {
        sendNotification("baseballEmoji");
    });

    auto skewedLinkerAgain = replaceWithSkewed(page2Ui.linkerAgain, -19.44, 0, 5.647, 0, 0);
    QObject::connect(skewedLinkerAgain, &QPushButton::clicked, []() {
        sendNotification("linkerAgain");
    });

    auto addHoverEffect = [](QPushButton *btn) {
        btn->setStyleSheet(
            "QPushButton { background-color: rgba(116, 125, 136, 100); color: black; }"
            "QPushButton:hover { background-color: rgba(130, 140, 150, 100); }"
            "QPushButton:pressed { background-color: rgba(100, 110, 120, 100); }"
        );
    };
    
    // page 1
    addHoverEffect(ui.someoneDied);
    addHoverEffect(ui.alarm);
    addHoverEffect(ui.weather);
    addHoverEffect(ui.redesign);
    addHoverEffect(ui.linkie);
    addHoverEffect(ui.page2);

    QObject::connect(ui.someoneDied, &QPushButton::clicked, []() {
        sendNotification("someoneDied");
    });

    QObject::connect(ui.alarm, &QPushButton::clicked, []() {
        sendNotification("serversDying");
    });

    QObject::connect(ui.weather, &QPushButton::clicked, []() {
        sendNotification("earthOnFire");
    });

    QObject::connect(ui.redesign, &QPushButton::clicked, []() {
        sendNotification("websiteRedesign");
    });

    QObject::connect(ui.linkie, &QPushButton::clicked, []() {
        sendNotification("linkerTragedy");
    });

    QObject::connect(ui.page2, &QPushButton::clicked, [stackedWidget]() {
        stackedWidget->setCurrentIndex(1);
    });

    // page 2

    addHoverEffect(page2Ui.bussinIndustries);
    addHoverEffect(page2Ui.hTile);
    addHoverEffect(page2Ui.jonathanPork);
    addHoverEffect(page2Ui.hGif);
    addHoverEffect(page2Ui.findMeOnline);
    addHoverEffect(page2Ui.googServices);
    addHoverEffect(page2Ui.flash);
    addHoverEffect(page2Ui.coffee);
    addHoverEffect(page2Ui.noskid);
    addHoverEffect(page2Ui.backButton);

    QObject::connect(page2Ui.bussinIndustries, &QPushButton::clicked, []() {
        sendNotification("bussinIndustries");
    });

    QObject::connect(page2Ui.hTile, &QPushButton::clicked, []() {
        sendNotification("hTile");
    });

    QObject::connect(page2Ui.jonathanPork, &QPushButton::clicked, []() {
        sendNotification("jonathanPork");
    });

    QObject::connect(page2Ui.hGif, &QPushButton::clicked, []() {
        sendNotification("hGif");
    });

    QObject::connect(page2Ui.findMeOnline, &QPushButton::clicked, []() {
        sendNotification("findMeOnline");
    });

    QObject::connect(page2Ui.googServices, &QPushButton::clicked, []() {
        sendNotification("googServices");
    });

    QObject::connect(page2Ui.flash, &QPushButton::clicked, []() {
        sendNotification("flash");
    });

    QObject::connect(page2Ui.coffee, &QPushButton::clicked, []() {
        sendNotification("mcafee");
    });

    QObject::connect(page2Ui.noskid, &QPushButton::clicked, []() {
        sendNotification("noskid");
    });

    QObject::connect(page2Ui.linkerAgain, &QPushButton::clicked, []() {
        sendNotification("linkerAgain");
    });

    QObject::connect(page2Ui.baseballEmoji, &QPushButton::clicked, []() {
        sendNotification("baseballEmoji");
    });

    QObject::connect(page2Ui.mazeGambled, &QPushButton::clicked, []() {
        sendNotification("mazeGambled");
    });

    QObject::connect(page2Ui.femboyLabs, &QPushButton::clicked, []() {
        sendNotification("femboyLabs");
    });

    QObject::connect(page2Ui.backButton, &QPushButton::clicked, [stackedWidget]() {
        stackedWidget->setCurrentIndex(0);
    });


    if (!family.isEmpty()) {
        QFont customFont(family);
        customFont.setPixelSize(36);
        customFont.setItalic(true);
        ui.welcomeTheNews->setFont(customFont);
        ui.donation->setFont(customFont);
    }

    // page 1
    addConsistentStyle(ui.someoneDied);
    addConsistentStyle(ui.alarm);
    addConsistentStyle(ui.weather);
    addConsistentStyle(ui.redesign);
    addConsistentStyle(ui.linkie);
    addConsistentStyle(ui.page2);

    // page 2
    addConsistentStyle(page2Ui.bussinIndustries);
    addConsistentStyle(page2Ui.hTile);
    addConsistentStyle(page2Ui.jonathanPork);
    addConsistentStyle(page2Ui.hGif);
    addConsistentStyle(page2Ui.findMeOnline);
    addConsistentStyle(page2Ui.googServices);
    addConsistentStyle(page2Ui.flash);
    addConsistentStyle(page2Ui.coffee);
    addConsistentStyle(page2Ui.noskid);
    addConsistentStyle(page2Ui.backButton);

    // auto toast from page 1
    QTimer *autoToastTimer = new QTimer();
    int currentToastIndex = 0;
    bool isAutoToastRunning = false;

    QPushButton *autoToastButton = new QPushButton("Start Auto Toast");
    autoToastButton->setGeometry(623, 76, 182, 293);
    autoToastButton->setParent(page1Window->centralWidget());
    addHoverEffect(autoToastButton);
    addConsistentStyle(autoToastButton);

    QWidget *intervalContainer = new QWidget(page1Window->centralWidget());
    intervalContainer->setGeometry(389, 343, 400, 100);

    QHBoxLayout *intervalLayout = new QHBoxLayout(intervalContainer);
    intervalLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *intervalTextLabel = new QLabel("Toast Interval:");
    intervalLayout->addWidget(intervalTextLabel);

    QSlider *intervalSlider = new QSlider(Qt::Horizontal);
    intervalSlider->setMinimum(0);
    intervalSlider->setMaximum(5000);
    intervalSlider->setValue(100);
    intervalSlider->setFixedWidth(200);
    intervalLayout->addWidget(intervalSlider);

    QLabel *intervalValueLabel = new QLabel("Interval: 100ms");
    intervalLayout->addWidget(intervalValueLabel);

    QObject::connect(intervalSlider, &QSlider::valueChanged, [intervalValueLabel, autoToastTimer, &isAutoToastRunning](int value) {
        intervalValueLabel->setText(QString("Interval: %1ms").arg(value));
        if (isAutoToastRunning && autoToastTimer->isActive()) {
            autoToastTimer->stop();
            autoToastTimer->setInterval(value);
            autoToastTimer->start();
        }
    });

    QObject::connect(autoToastTimer, &QTimer::timeout, [&currentToastIndex]() {
        switch(currentToastIndex) {
            case 0: sendNotification("someoneDied"); break;
            case 1: sendNotification("donate"); break;
            case 2: sendNotification("serversDying"); break;
            case 3: sendNotification("deleteSystem32"); break;
            case 4: sendNotification("incomingCall"); break;
            case 5: sendNotification("earthOnFire"); break;
            case 6: sendNotification("friendRequest"); break;
            case 7: sendNotification("websiteRedesign"); break;
            case 8: sendNotification("roadblocks"); break;
            case 9: sendNotification("linkerTragedy"); break;
        }
        currentToastIndex = (currentToastIndex + 1) % 10;
    });

    QObject::connect(autoToastButton, &QPushButton::clicked, [autoToastButton, autoToastTimer, intervalSlider, &isAutoToastRunning, &currentToastIndex]() {
        if (!isAutoToastRunning) {
            isAutoToastRunning = true;
            currentToastIndex = 0;
            autoToastTimer->setInterval(intervalSlider->value());
            autoToastTimer->start();
            autoToastButton->setText("Stop Auto Toast");
        } else {
            isAutoToastRunning = false;
            autoToastTimer->stop();
            autoToastButton->setText("Start Auto Toast");
        }
    });

    window.show();
    return app.exec();
}