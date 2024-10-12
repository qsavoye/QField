#include "egenioussreceiver.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonValue>

EgenioussReceiver::EgenioussReceiver( QObject *parent )
  : AbstractGnssReceiver( parent ), mTcpSocket( new QTcpSocket() )
{
  connect( mTcpSocket, &QTcpSocket::readyRead, this, &EgenioussReceiver::onReadyRead );
  connect( mTcpSocket, &QTcpSocket::errorOccurred, this, &EgenioussReceiver::handleError );
  connect( mTcpSocket, &QTcpSocket::connected, this, &EgenioussReceiver::connected );
  connect( mTcpSocket, &QTcpSocket::disconnected, this, &EgenioussReceiver::disconnected );
  setValid( true );
}

void EgenioussReceiver::handleConnectDevice()
{
  mTcpSocket->connectToHost( mAddress, mPort, QTcpSocket::ReadWrite );
}

void EgenioussReceiver::connected()
{
  mSocketState = QAbstractSocket::ConnectedState;
  mSocketStateString = tr( "Successfully connected" );
  emit socketStateChanged( mSocketState );
  setValid( true );
}

void EgenioussReceiver::handleDisconnectDevice()
{
  mTcpSocket->disconnectFromHost();
}

void EgenioussReceiver::disconnected()
{
  if ( mTcpSocket->state() == QAbstractSocket::ConnectedState )
  {
    mSocketState = QAbstractSocket::UnconnectedState;
    mSocketStateString = tr( "Disconnected" );
    emit socketStateChanged( mSocketState );
  }
}

QList<QPair<QString, QVariant>> EgenioussReceiver::details()
{
  QList<QPair<QString, QVariant>> detailsList;

  if ( mPayload.isEmpty() )
  {
    return detailsList;
  }

  detailsList.append( qMakePair( "q", mPayload.value( "q" ).toDouble() ) );

  return detailsList;
}

void EgenioussReceiver::onReadyRead()
{
  const int minimumDataSize = 9;
  const uint8_t validStartByte = 0xFE;
  const int payloadHeaderSize = 8;

  QByteArray mReceivedData = mTcpSocket->readAll();
  if ( mReceivedData.size() < minimumDataSize )
  {
    mLastError = tr( "Received data is too short to process" );
    emit lastErrorChanged( mLastError );
    return;
  }
  if ( static_cast<uint8_t>( mReceivedData[0] ) != validStartByte )
  {
    mLastError = tr( "Invalid start byte" );
    emit lastErrorChanged( mLastError );
    return;
  }
  uint32_t payloadLength;
  QDataStream dataStream( mReceivedData.mid( 4, 4 ) );
  dataStream.setByteOrder( QDataStream::LittleEndian );
  dataStream >> payloadLength;
  if ( mReceivedData.size() < payloadHeaderSize + payloadLength )
  {
    mLastError = tr( "Received data is too short to contain the payload" );
    emit lastErrorChanged( mLastError );
    return;
  }
  QJsonDocument jsonDoc = QJsonDocument::fromJson( mReceivedData.mid( payloadHeaderSize, payloadLength ) );
  if ( jsonDoc.isNull() || !jsonDoc.isObject() )
  {
    mLastError = tr( "Failed to parse JSON" );
    emit lastErrorChanged( mLastError );
    return;
  }
  mPayload = jsonDoc.object();
  const double latitude = mPayload.value( "lat" ).toDouble() == 0 ? std::numeric_limits<double>::quiet_NaN() : mPayload.value( "lat" ).toDouble();
  const double longitude = mPayload.value( "lon" ).toDouble() == 0 ? std::numeric_limits<double>::quiet_NaN() : mPayload.value( "lon" ).toDouble();
  const double elevation = mPayload.value( "alt" ).toDouble() == 0 ? std::numeric_limits<double>::quiet_NaN() : mPayload.value( "alt" ).toDouble();
  mLastGnssPositionInformation = GnssPositionInformation(
    latitude,
    longitude,
    elevation,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    QList<QgsSatelliteInfo>(),
    0,
    0,
    0,
    std::numeric_limits<double>::quiet_NaN(),
    std::numeric_limits<double>::quiet_NaN(),
    QDateTime::fromMSecsSinceEpoch( mPayload.value( "utc" ).toDouble() / 1e6, Qt::UTC ),
    QChar(),
    0,
    1 );

  emit lastGnssPositionInformationChanged( mLastGnssPositionInformation );
}

void EgenioussReceiver::handleError( QAbstractSocket::SocketError error )
{
  switch ( error )
  {
    case QAbstractSocket::HostNotFoundError:
      mLastError = tr( "Could not find the remote host" );
      break;
    case QAbstractSocket::NetworkError:
      mLastError = tr( "Attempt to read or write from socket returned an error" );
      break;
    case QAbstractSocket::ConnectionRefusedError:
      mLastError = tr( "The connection was refused by the remote host" );
      break;
    default:
      mLastError = tr( "TCP receiver error (%1)" ).arg( QMetaEnum::fromType<QAbstractSocket::SocketError>().valueToKey( error ) );
      break;
  }
  mSocketState = QAbstractSocket::UnconnectedState;
  mSocketStateString = mLastError;
  qInfo() << QStringLiteral( "EgenioussReceiver: Error: %1" ).arg( mLastError );

  emit lastErrorChanged( mLastError );
}
