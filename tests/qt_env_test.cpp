// Qt 环境验证测试程序
// 用于验证 Qt 6 开发环境配置正确

#include <QApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QDebug>
#include <QtGlobal>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 输出 Qt 版本信息
    qDebug() << "Qt Version:" << QT_VERSION_STR;
    qDebug() << "Qt Runtime Version:" << qVersion();
    
    // 创建测试窗口
    QWidget window;
    window.setWindowTitle("SuYan Qt Environment Test");
    window.setMinimumSize(400, 200);
    
    QVBoxLayout *layout = new QVBoxLayout(&window);
    
    QLabel *titleLabel = new QLabel("Qt 环境验证成功!");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #007AFF;");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QString versionInfo = QString("Qt Version: %1\nCompiled with: %2")
        .arg(qVersion())
        .arg(QT_VERSION_STR);
    QLabel *versionLabel = new QLabel(versionInfo);
    versionLabel->setAlignment(Qt::AlignCenter);
    
    QLabel *statusLabel = new QLabel("✓ Qt Core\n✓ Qt Gui\n✓ Qt Widgets");
    statusLabel->setStyleSheet("color: green;");
    statusLabel->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addWidget(statusLabel);
    
    window.show();
    
    // 非交互模式：直接退出并返回成功
    if (argc > 1 && QString(argv[1]) == "--test") {
        qDebug() << "Qt environment test passed!";
        return 0;
    }
    
    return app.exec();
}
