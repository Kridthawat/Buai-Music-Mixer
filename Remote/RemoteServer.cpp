#include "Remote/RemoteServer.h"

#include <QApplication>
#include <QBuffer>
#include <QCryptographicHash>
#include <QHostAddress>
#include <QImage>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNetworkInterface>
#include <QPixmap>
#include <QRandomGenerator>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <QWheelEvent>

static const int    kMaxRequest  = 64 * 1024;
static const int    kMaxFrameW   = 1000;
static const qint64 kClientAlive = 4000;

static const char *kPage = R"HTML(<!doctype html>
<html><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no,viewport-fit=cover">
<meta name="apple-mobile-web-app-capable" content="yes">
<title>Buai Music Mixer</title>
<style>
html,body{margin:0;height:100%;background:#07080d;color:#7dffc0;font:13px sans-serif;overflow:hidden;touch-action:none;overscroll-behavior:none}
#wrap{position:fixed;left:0;top:0;right:0;bottom:0;display:flex;align-items:center;justify-content:center}
#s{max-width:100%;max-height:100%;touch-action:none;-webkit-user-select:none;user-select:none;-webkit-touch-callout:none;-webkit-user-drag:none}
#msg{position:fixed;left:0;right:0;top:0;text-align:center;padding:6px;background:rgba(0,0,0,.75);display:none;z-index:3}
#bar{position:fixed;right:6px;bottom:6px;display:flex;gap:6px;z-index:2;opacity:.75}
button{background:#14202a;color:#7dffc0;border:1px solid #1f5a45;border-radius:8px;padding:8px 12px;font-size:13px}
</style></head><body>
<div id="msg"></div>
<div id="wrap"><img id="s" alt=""></div>
<div id="bar"><button id="esc">Esc</button><button id="fs">&#9974;</button></div>
<script>
var T=new URLSearchParams(location.search).get('t')||'';
var img=document.getElementById('s'),msg=document.getElementById('msg');
var h='',fails=0,dead=false;
function say(t){if(t){msg.textContent=t;msg.style.display='block';}else{msg.style.display='none';}}
function loop(){
  if(dead)return;
  fetch('/f?t='+encodeURIComponent(T)+'&h='+h,{cache:'no-store'}).then(function(r){
    if(r.status==403){dead=true;say('QRหมดอายุ/ปิดอยู่ กรุณาสแกนใหม่');return;}
    if(r.status==200){
      h=r.headers.get('X-H')||'';
      return r.blob().then(function(b){
        var u=URL.createObjectURL(b),old=img.src;
        img.onload=function(){if(old&&old.indexOf('blob:')==0)URL.revokeObjectURL(old);};
        img.src=u;
      });
    }
  }).then(function(){fails=0;say('');setTimeout(loop,50);})
    .catch(function(){fails++;say('กำลังเชื่อมต่อ...');setTimeout(loop,Math.min(2000,250*fails));});
}
loop();

var pend=[],sending=false;
function pump(){
  if(sending||!pend.length)return;
  sending=true;
  var o=pend.shift();
  fetch('/e?t='+encodeURIComponent(T),{method:'POST',body:new URLSearchParams(o)})
    .catch(function(){}).then(function(){sending=false;pump();});
}
function send(o){
  if(o.type=='move'&&pend.length&&pend[pend.length-1].type=='move')pend[pend.length-1]=o;
  else pend.push(o);
  pump();
}
function pos(e){
  var r=img.getBoundingClientRect();
  var x=(e.clientX-r.left)/r.width,y=(e.clientY-r.top)/r.height;
  return {x:Math.max(0,Math.min(1,x)),y:Math.max(0,Math.min(1,y))};
}
var pts={},n=0,down=false,last=null,sy=0,acc=0;
img.addEventListener('pointerdown',function(e){
  e.preventDefault();img.setPointerCapture(e.pointerId);
  pts[e.pointerId]=e;n=Object.keys(pts).length;
  if(n==1){var p=pos(e);last=p;down=true;send({type:'down',x:p.x,y:p.y});}
  else if(n==2){
    if(down){send({type:'up',x:last.x,y:last.y});down=false;}
    var k=Object.keys(pts);sy=(pts[k[0]].clientY+pts[k[1]].clientY)/2;acc=0;
  }
});
img.addEventListener('pointermove',function(e){
  if(!pts[e.pointerId])return;
  e.preventDefault();pts[e.pointerId]=e;
  if(n==1&&down){var p=pos(e);last=p;send({type:'move',x:p.x,y:p.y});}
  else if(n==2){
    var k=Object.keys(pts);var cy=(pts[k[0]].clientY+pts[k[1]].clientY)/2;
    acc+=cy-sy;sy=cy;
    if(Math.abs(acc)>10){var p2=pos(pts[k[0]]);send({type:'wheel',x:p2.x,y:p2.y,dy:Math.round(acc)});acc=0;}
  }
});
function end(e){
  if(!pts[e.pointerId])return;
  e.preventDefault();
  var wasOne=(n==1);
  delete pts[e.pointerId];n=Object.keys(pts).length;
  if(wasOne&&down){var p=pos(e);send({type:'up',x:p.x,y:p.y});down=false;}
}
img.addEventListener('pointerup',end);
img.addEventListener('pointercancel',end);
img.addEventListener('contextmenu',function(e){e.preventDefault();});
document.addEventListener('touchmove',function(e){e.preventDefault();},{passive:false});
document.getElementById('esc').onclick=function(){send({type:'key',key:'esc'});};
document.getElementById('fs').onclick=function(){
  var d=document.documentElement;
  if(d.requestFullscreen)d.requestFullscreen();else if(d.webkitRequestFullscreen)d.webkitRequestFullscreen();
};
</script></body></html>
)HTML";

// ---------------------------------------------------------------------------

RemoteServer::RemoteServer(QWidget *mainWindow, QObject *parent)
    : QObject(parent), mainWin(mainWindow)
{
    clock.start();
    lastGrab.start();
    connect(&server, &QTcpServer::newConnection, this, &RemoteServer::onNewConnection);
}

RemoteServer::~RemoteServer()
{
    stop();
}

QStringList RemoteServer::localAddresses()
{
    QStringList prefer, other;

    const QList<QNetworkInterface> ifs = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &ni : ifs)
    {
        if (!(ni.flags() & QNetworkInterface::IsUp) ||
            !(ni.flags() & QNetworkInterface::IsRunning) ||
            (ni.flags() & QNetworkInterface::IsLoopBack))
            continue;

        const QList<QNetworkAddressEntry> entries = ni.addressEntries();
        for (const QNetworkAddressEntry &e : entries)
        {
            QHostAddress a = e.ip();
            if (a.protocol() != QAbstractSocket::IPv4Protocol)
                continue;

            QString s = a.toString();
            if (s.startsWith("169.254."))       // link-local, useless
                continue;

            quint32 v = a.toIPv4Address();
            bool priv = (v >> 24) == 10 ||
                        (v >> 20) == 0xAC1 ||           // 172.16/12
                        (v >> 16) == 0xC0A8;            // 192.168/16
            if (priv) prefer << s; else other << s;
        }
    }
    return prefer + other;
}

bool RemoteServer::start(const QString &ip)
{
    stop();

    ipText = ip;

    // random token (the QR code carries it)
    static const char alphabet[] = "abcdefghijkmnpqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";
    token.clear();
    for (int i = 0; i < 16; i++)
        token += QChar(alphabet[QRandomGenerator::global()->bounded((int)sizeof(alphabet) - 1)]);

    for (quint16 p = 8765; p < 8775; p++)
    {
        if (server.listen(QHostAddress::AnyIPv4, p))
            return true;
    }
    return false;
}

void RemoteServer::stop()
{
    if (server.isListening())
        server.close();

    // drop all open connections
    const QList<QTcpSocket *> socks = findChildren<QTcpSocket *>();
    for (QTcpSocket *s : socks)
    {
        s->disconnect(this);
        s->abort();
        s->deleteLater();
    }

    token.clear();
    clients.clear();
    dragWindow = nullptr;
    pressWidget = nullptr;
    emit clientsChanged();
}

QString RemoteServer::url() const
{
    if (!server.isListening())
        return QString();
    return QString("http://%1:%2/?t=%3").arg(ipText).arg(server.serverPort()).arg(token);
}

int RemoteServer::clientCount() const
{
    qint64 now = clock.elapsed();
    int n = 0;
    for (auto it = clients.constBegin(); it != clients.constEnd(); ++it)
        if (now - it.value() < kClientAlive)
            n++;
    return n;
}

void RemoteServer::touchClient(const QString &ip)
{
    bool isNew = !clients.contains(ip) || (clock.elapsed() - clients.value(ip) >= kClientAlive);
    clients[ip] = clock.elapsed();
    if (isNew)
        emit clientsChanged();
}

// ---------------------------------------------------------------------------
// HTTP
// ---------------------------------------------------------------------------

void RemoteServer::onNewConnection()
{
    while (server.hasPendingConnections())
    {
        QTcpSocket *s = server.nextPendingConnection();
        s->setParent(this);

        connect(s, &QTcpSocket::readyRead, this, [this, s]() { onReadyRead(s); });
        connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);

        // never keep a half-open connection around
        QTimer::singleShot(8000, s, [s]() {
            if (s->state() != QAbstractSocket::UnconnectedState)
                s->abort();
        });
    }
}

void RemoteServer::onReadyRead(QTcpSocket *s)
{
    QByteArray buf = s->property("buf").toByteArray();
    buf += s->readAll();

    if (buf.size() > kMaxRequest)
    {
        s->abort();
        return;
    }

    int headEnd = buf.indexOf("\r\n\r\n");
    if (headEnd < 0)
    {
        s->setProperty("buf", buf);
        return;
    }

    QByteArray head = buf.left(headEnd);
    QList<QByteArray> lines = head.split('\n');
    QList<QByteArray> reqLine = lines.value(0).trimmed().split(' ');
    if (reqLine.size() < 2)
    {
        s->abort();
        return;
    }

    int contentLength = 0;
    for (int i = 1; i < lines.size(); i++)
    {
        QByteArray l = lines[i].trimmed();
        if (l.toLower().startsWith("content-length:"))
            contentLength = l.mid(15).trimmed().toInt();
    }

    if (contentLength < 0 || contentLength > kMaxRequest)
    {
        s->abort();
        return;
    }

    QByteArray body = buf.mid(headEnd + 4);
    if (body.size() < contentLength)
    {
        s->setProperty("buf", buf);
        return;
    }
    body = body.left(contentLength);
    s->setProperty("buf", QByteArray());

    QByteArray target = reqLine[1];
    QByteArray path = target;
    QByteArray query;
    int q = target.indexOf('?');
    if (q >= 0)
    {
        path = target.left(q);
        query = target.mid(q + 1);
    }

    handle(s, reqLine[0], QString::fromLatin1(path), query, body);
}

void RemoteServer::reply(QTcpSocket *s, int code, const QByteArray &contentType,
                         const QByteArray &body, const QByteArray &extraHeaders)
{
    QByteArray status;
    switch (code) {
    case 200: status = "OK"; break;
    case 204: status = "No Content"; break;
    case 403: status = "Forbidden"; break;
    default:  status = "Not Found"; code = 404; break;
    }

    QByteArray out = "HTTP/1.1 " + QByteArray::number(code) + " " + status + "\r\n";
    out += "Content-Type: " + contentType + "\r\n";
    out += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    out += "Cache-Control: no-store\r\n";
    out += "Connection: close\r\n";
    out += "Access-Control-Expose-Headers: X-H\r\n";
    out += extraHeaders;
    out += "\r\n";
    out += body;

    s->write(out);
    s->disconnectFromHost();
}

void RemoteServer::handle(QTcpSocket *s, const QByteArray &method, const QString &path,
                          const QByteArray &query, const QByteArray &body)
{
    QUrlQuery uq{QString::fromLatin1(query)};

    if (token.isEmpty() || uq.queryItemValue("t") != token)
    {
        reply(s, 403, "text/plain; charset=utf-8", "Forbidden");
        return;
    }

    QString ip = s->peerAddress().toString();
    if (ip.startsWith("::ffff:"))
        ip = ip.mid(7);

    if (path == "/")
    {
        touchClient(ip);
        reply(s, 200, "text/html; charset=utf-8", QByteArray(kPage));
        return;
    }

    if (path == "/f")
    {
        touchClient(ip);

        if (!grabFrame())
        {
            reply(s, 204, "text/plain", QByteArray());
            return;
        }

        if (uq.queryItemValue("h") == lastHash)
        {
            reply(s, 204, "text/plain", QByteArray());
            return;
        }

        reply(s, 200, "image/jpeg", lastJpeg, "X-H: " + lastHash.toLatin1() + "\r\n");
        return;
    }

    if (path == "/e" && method == "POST")
    {
        touchClient(ip);

        QMap<QString, QString> p;
        QUrlQuery bq{QString::fromUtf8(body)};
        const QList<QPair<QString, QString> > items = bq.queryItems(QUrl::FullyDecoded);
        for (const QPair<QString, QString> &it : items)
            p.insert(it.first, it.second);

        inject(p);
        reply(s, 200, "text/plain", "ok");
        return;
    }

    reply(s, 404, "text/plain", "Not found");
}

// ---------------------------------------------------------------------------
// screen mirroring
// ---------------------------------------------------------------------------

QWidget *RemoteServer::targetWindow() const
{
    if (QWidget *w = QApplication::activePopupWidget())
        return w;
    if (QWidget *w = QApplication::activeModalWidget())
        if (!w->property("buaiRemoteSkip").toBool())
            return w;

    QWidget *a = QApplication::activeWindow();
    if (a && a->isVisible() && !a->isMinimized() && !a->property("buaiRemoteSkip").toBool())
        return a;

    return mainWin;
}

bool RemoteServer::grabFrame()
{
    // share one capture between several phones / fast polling
    if (!lastJpeg.isEmpty() && lastGrab.elapsed() < 40)
        return true;

    QWidget *w = dragWindow ? dragWindow.data() : targetWindow();
    if (!w || w->width() < 2 || w->height() < 2)
        return false;

    QImage img = w->grab().toImage().convertToFormat(QImage::Format_RGB32);
    if (img.isNull())
        return false;

    if (img.width() > kMaxFrameW)
        img = img.scaledToWidth(kMaxFrameW, Qt::SmoothTransformation);

    QCryptographicHash md5(QCryptographicHash::Md5);
    md5.addData(reinterpret_cast<const char *>(img.constBits()), int(img.sizeInBytes()));
    // the source window identity / size is part of the hash
    md5.addData(QByteArray::number(reinterpret_cast<quintptr>(w)));
    md5.addData(QByteArray::number(img.width()) + "x" + QByteArray::number(img.height()));
    QString hash = QString::fromLatin1(md5.result().toHex().left(16));

    lastGrab.restart();

    if (hash == lastHash && !lastJpeg.isEmpty())
        return true;

    QByteArray jpg;
    QBuffer buf(&jpg);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "JPEG", 62);

    lastJpeg = jpg;
    lastHash = hash;
    return !lastJpeg.isEmpty();
}

// ---------------------------------------------------------------------------
// input injection
// ---------------------------------------------------------------------------

void RemoteServer::inject(const QMap<QString, QString> &p)
{
    const QString type = p.value("type");

    if (type == "key")
    {
        if (p.value("key") == "esc")
        {
            QWidget *w = QApplication::activePopupWidget();
            if (!w) w = QApplication::focusWidget();
            if (!w) w = targetWindow();
            QKeyEvent press(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QKeyEvent release(QEvent::KeyRelease, Qt::Key_Escape, Qt::NoModifier);
            QApplication::sendEvent(w, &press);
            QApplication::sendEvent(w, &release);
            lastJpeg.clear();
        }
        return;
    }

    double nx = qBound(0.0, p.value("x").toDouble(), 1.0);
    double ny = qBound(0.0, p.value("y").toDouble(), 1.0);

    QWidget *win = nullptr;
    if (type == "down")
        win = targetWindow();
    else if (dragWindow)
        win = dragWindow.data();
    else
        win = targetWindow();

    if (!win)
        return;

    QPoint wp(qBound(0, qRound(nx * win->width()),  win->width()  - 1),
              qBound(0, qRound(ny * win->height()), win->height() - 1));

    if (type == "down")
    {
        QWidget *child = win->childAt(wp);
        if (!child)
            child = win;

        dragWindow = win;
        pressWidget = child;

        QPoint lp = child->mapFrom(win, wp);
        QPoint gp = win->mapToGlobal(wp);
        QMouseEvent ev(QEvent::MouseButtonPress, QPointF(lp), QPointF(gp),
                       Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(child, &ev);
    }
    else if (type == "move")
    {
        if (!pressWidget)
            return;
        QPoint lp = pressWidget->mapFrom(win, wp);
        QPoint gp = win->mapToGlobal(wp);
        QMouseEvent ev(QEvent::MouseMove, QPointF(lp), QPointF(gp),
                       Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
        QApplication::sendEvent(pressWidget.data(), &ev);
    }
    else if (type == "up")
    {
        if (pressWidget)
        {
            QWidget *pw = pressWidget.data();
            QPoint lp = pw->mapFrom(win, wp);
            QPoint gp = win->mapToGlobal(wp);
            QMouseEvent ev(QEvent::MouseButtonRelease, QPointF(lp), QPointF(gp),
                           Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
            QApplication::sendEvent(pw, &ev);
        }
        pressWidget = nullptr;
        dragWindow = nullptr;
    }
    else if (type == "wheel")
    {
        QWidget *child = win->childAt(wp);
        if (!child)
            child = win;
        QPoint lp = child->mapFrom(win, wp);
        QPoint gp = win->mapToGlobal(wp);
        int dy = p.value("dy").toInt() * 3;
        QWheelEvent ev(QPointF(lp), QPointF(gp), QPoint(0, 0), QPoint(0, dy),
                       Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QApplication::sendEvent(child, &ev);
    }

    // the screen changes because of the input: force a fresh capture
    lastJpeg.clear();
}
