#include "mainwindow.h"
#include "ui_mainwindow.h" // DO NOT REMOVE - this is for the ui form
#include <QTime>
#include <QtMultimedia> //videoplayer lib
#include <QVideoSurfaceFormat>
#include <QGraphicsVideoItem>
#include <QVideoWidget>

using namespace std;
/*2013/2014
 *Megan Gardner
 *for ELEC 490
 *Group Members:    Kevin Cook
 *                  Megan Gardner
 *                  Cameron Kramer
 *
 *2012/2013
  Joey Frohlinger
  for ELEC 490
  Group Members:    Bren Piper
                    Adam Bunn
                    Joey Frohlinger
  Supervisor:       Dr. Evelyn Morin

  */

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m(QSize(400,400),QImage::Format_RGB32),//constructor for left heatmap
    m2(QSize(400,400),QImage::Format_RGB32) // constructor for right heat map
  //  angleFile("C:\\Users\\Megan Gardner\\GitHub\\490\\angleLog.log",ios::out)
{
    ui->setupUi(this);

    leftArrow = new QShortcut(Qt::Key_Left,this, SLOT(leftArrowSlot()));
    rightArrow = new QShortcut(Qt::Key_Right,this,SLOT(rightArrowSlot()));

    QGridLayout *grid = new QGridLayout(ui->vidWidget);
    grid->setSpacing(20);
    videoItem = new QGraphicsVideoItem();
    videoItem->setSize(QSizeF(640,480));

    QGraphicsScene *sceneVid = new QGraphicsScene(this);
    QGraphicsView *graphicsview = new QGraphicsView(sceneVid);

//    QVideoWidget *videoWidget = new QVideoWidget(this);

//    grid->addWidget(videoWidget,1,0,3,1);
//    videoWidget->show();
 //   mediaPlayer.setVideoOutput(videoWidget);
    sceneVid->addItem(videoItem);
    grid->addWidget(graphicsview,1,0,3,1); //adds video scene widget to GUI

    mediaPlayer.setVideoOutput(videoItem);

    QGridLayout *gridS = new QGridLayout(ui->seekWidget);
    gridS->setSpacing(20);
    vidSeek = new QSlider(Qt::Horizontal,ui->seekWidget); //video seek slider
    vidSeek->setRange(0,0);
    gridS->addWidget(vidSeek,1,0,3,1);

    rotate = new QSlider(Qt::Vertical,ui->rotateWidget);
    rotate->setRange(-180,180);
    rotate->setValue(0);
    connect(rotate,SIGNAL(valueChanged(int)),this,SLOT(rotateVideo(int))); //rotate sider - for upside down movs
    gridS->addWidget(rotate,0,20,0,1);

    comm = 0;
    commThread = new QThread(this);
    uiInit();//this function is to initialize the data in the UI (boxes etc)

    //timer based interrupt for screen rendering
    QTimer* timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start(1000/60); //60 hz frame rate (or so)

    //timer for comm thread
    QTimer* timer2 = new QTimer(this);
    connect(timer2, SIGNAL(timeout()), this, SLOT(commTimer()));
    timer2->start(10);

    //timer for video ticker
    QTimer* timer3 = new QTimer(this);
    connect(timer3, SIGNAL(timeout()), this, SLOT(vidTime()));
    timer3->start(10);

    LFootMask.load("c:/Users/Megan Gardner/GitHub/490/leftFootMask.png"); // location of foot mask
    RFootMask.load("c:/Users/Megan Gardner/GitHub/490/rightFootMask.png");
    Lscene = new QGraphicsScene(); //create empty scene
    Rscene = new QGraphicsScene();

    // Get foot data
   /* if(comm->isEnabled()) //is COM available on start up?
    {
        vec = comm->getData();
        vec2 = new vector<DataPoint>;
    }
    else
    {*/
        vec = new vector<DataPoint>;
      //  vec = comm->getData();
        vec2 = new vector<DataPoint>;
   // }

    m.genMap(*vec); // left foot heat map
    m2.genMap(*vec2); // right foot heat map

    Lscene->setSceneRect(m.rect()); //set the scene's view rectangle to the image's
    pix = QPixmap::fromImage(m); //create a pixmap from the image
    pix = pix.scaled(ui->renderView2->size());
    pixItem = Lscene->addPixmap(pix); //add the pixmap to the scene
    ui->renderView2->setSceneRect(pix.rect()); //set the renderviews view rectangle
    ui->renderView2->setScene(Lscene); //set the renderViews scene

    Rscene->setSceneRect(m2.rect());
    pix2 = QPixmap::fromImage(m2);
    pix2 = pix2.scaled(ui->renderView->size());
    pixItem2 = Rscene->addPixmap(pix2);
    ui->renderView->setSceneRect(pix2.rect());
    ui->renderView->setScene(Rscene);

    //kneeAngle1.setNum(0);
    ui->angleOut->setPlainText(0);

    commThread->start();
    angleFile << "begin" << endl;
}

//update called from timer thread to lock frame rate
void MainWindow::update(){
    cout << "in update" << endl;

   // comm->update();
    //vec = comm->getData();
    //k1 = comm->getAngleData1();
    //k2 = comm->getAngleData2();
    float knee = k2.KneeAngle(k1); //always call in this fashion - otherwise calculation will be wrong
    angleFile<<knee<<endl;
    QString text = 0; //used to initialize - DO NOT CHANGE
    text.setNum(45);

    // update angle
    ui->angleOut->setPlainText(text);

    //update feet
    m.genMap(*vec);
    m.applyMask(LFootMask);
    Lscene->removeItem(pixItem);
    delete pixItem; //memory leak fix (What what!)
    pix = QPixmap::fromImage(m);
    pixItem = Lscene->addPixmap(pix);

    m2.genMap(*vec2);
    m2.applyMask(RFootMask);
    Rscene->removeItem(pixItem2);
    delete pixItem2;
    pix2 = QPixmap::fromImage(m2);
    pixItem2 = Rscene->addPixmap(pix2);

}


void MainWindow::uiInit()
{
    ui->comPortBox->addItems(Communication::getPortsList());
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_comPortBox_currentIndexChanged(const QString &arg1)
{
    currentComPort = arg1;
    changeCom();
}
void MainWindow::commTimer(){
    if(!commThread->isRunning())
    commThread->start();
}

void MainWindow::changeCom(){
    if(comm != 0){
        cout<<"com open already - killing"<<endl;
        commThread->terminate();
        delete comm;
        comm = 0;
    }
//    cout<<"Called"<<arg1.toStdString()<<endl;
    comm = new Communication(currentComPort.toStdString());
    comm->moveToThread(commThread);
    connect(commThread, SIGNAL(started()), comm, SLOT(update()));
    commThread->start();
    vec = comm->getData(); //pass data pointer
}


void MainWindow::on_vidPlay_clicked()
{
    mediaPlayer.play();
}

void MainWindow::vidTime(){
    static qint64 time;
    static int h;
    static int s;
    static int m;
    static int ms;

    time = mediaPlayer.duration(); //elapsed time in milliseconds - updated to reflect QMediaPlayer functions
    ms = time%1000;
    s = (time/1000) % 60;
    m = (time/60000)%60;
    h = (time/3600000);
    QTime qtime(h,m,s,ms);
    ui->timeEdit->setTime(qtime);
    if(ui->loopBox->isChecked()){
        if(qtime > ui->vidEndLoop->time()){
            ms = ui->vidStartLoop->time().msec();
            s = ui->vidStartLoop->time().second()*1000;
            m = ui->vidStartLoop->time().minute()*60000;
            h = ui->vidStartLoop->time().hour()*360000;
            mediaPlayer.setPosition(h+m+s+ms); // updated to reflect QMediaPlayer functions
        }
    }
}

void MainWindow::on_vidPath_textEdited(const QString &arg1)
{
    vidPathText = arg1;
}


void MainWindow::on_vidPause_clicked()
{
    mediaPlayer.pause();

}

void MainWindow::on_vidPath_returnPressed()
{
    on_vidPlay_clicked();
}

void MainWindow::on_vidStartLoopSet_clicked()
{
    ui->vidStartLoop->setTime(ui->timeEdit->time());
}

void MainWindow::on_vidEndLoopSet_clicked()
{
    ui->vidEndLoop->setTime(ui->timeEdit->time());
}

void MainWindow::leftArrowSlot(){
    if(mediaPlayer.duration()-33 > 0)    //check if we can skip back 33 ms
    {
        mediaPlayer.setPosition(mediaPlayer.duration()-33);
    }
    else
    {
        mediaPlayer.setPosition(0);
    }
}

void MainWindow::rightArrowSlot(){
    if(mediaPlayer.duration()+33 < mediaPlayer.duration())
        mediaPlayer.setPosition(mediaPlayer.duration() + 33);
}

void MainWindow::on_vidStop_clicked()
{
    mediaPlayer.stop();
}

void MainWindow::on_comPortBox_activated(const QString &arg1)
{
     cout<<"Called"<<arg1.toStdString()<<endl;
    if(comm != 0){
        cout<<"com open already - killing"<<endl;
        commThread->terminate();
        delete comm;
        comm = 0;
    }
    else{
    }

    for(int i=0; i<1; i++){
    comm = new Communication(arg1.toStdString());
    comm->moveToThread(commThread);
    connect(commThread, SIGNAL(started()), comm, SLOT(update()));
    commThread->start();
    vec = comm->getData(); //pass data pointer
    }
    cout<<"opened " << arg1.toStdString()<<endl;
}

void MainWindow::on_fileBrowserButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"));
    ui->vidPath->setText(fileName);
    vidPathText = fileName;
    mediaPlayer.setMedia(QUrl::fromLocalFile(fileName)); // load media to mediaPlayer
}

void MainWindow::on_MainWindow_destroyed()
{
    cout<<"Exit"<<endl;
}

void MainWindow::rotateVideo(int angle) // rotate video
{
    qreal x = videoItem->boundingRect().width()/2.0;
    qreal y = videoItem->boundingRect().height()/2.0;
    videoItem->setTransform(QTransform().translate(x,y).rotate(angle).translate(-x,-y));

}
