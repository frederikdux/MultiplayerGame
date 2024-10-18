//
// Created by Frederik on 12.10.2024.
//

#ifndef MESSAGETYPES_H
#define MESSAGETYPES_H

enum class MessageType {
    Message,
    SendName,
    Id,
    NewEnemy,
    RemoveEnemy,
    UpdateEnemyName,
    UpdateEnemyHealth,
    EnemyDied,
    NewBullet,
    RemoveBullet,
    EnemyRespawned,
    UpdateEnemyPosition,
    UpdateEnemyPositionAndVelocity,
    UpdateEnemyPosVelRot,
    RequestGameUpdate,
    RequestRespawn,
    Unknown
};

inline std::string messageTypeToString(MessageType type) {
    switch (type) {
        case MessageType::Message:
            return "Message";
        case MessageType::SendName:
            return "SendName";
        case MessageType::Id:
            return "Id";
        case MessageType::NewEnemy:
            return "NewEnemy";
        case MessageType::RemoveEnemy:
            return "RemoveEnemy";
        case MessageType::UpdateEnemyName:
            return "UpdateEnemyName";
        case MessageType::UpdateEnemyHealth:
            return "UpdateEnemyHealth";
        case MessageType::EnemyDied:
            return "EnemyDied";
        case MessageType::NewBullet:
            return "NewBullet";
        case MessageType::RemoveBullet:
            return "RemoveBullet";
        case MessageType::EnemyRespawned:
            return "EnemyRespawned";
        case MessageType::UpdateEnemyPosition:
            return "UpdateEnemyPosition";
        case MessageType::UpdateEnemyPositionAndVelocity:
            return "UpdateEnemyPositionAndVelocity";
        case MessageType::UpdateEnemyPosVelRot:
            return "UpdateEnemyPosVelRot";
        case MessageType::RequestGameUpdate:
            return "RequestGameUpdate";
        case MessageType::RequestRespawn:
            return "RequestRespawn";
        case MessageType::Unknown:
            return "Unknown";
        default:
            return "Unknown";
    }
}

inline MessageType stringToMessageType(const std::string& type) {
    if (type == "Message") {
        return MessageType::Message;
    } else if (type == "SendName") {
        return MessageType::SendName;
    } else if (type == "Id") {
        return MessageType::Id;
    } else if (type == "NewEnemy") {
        return MessageType::NewEnemy;
    } else if (type == "RemoveEnemy") {
        return MessageType::RemoveEnemy;
    } else if (type == "UpdateEnemyName") {
        return MessageType::UpdateEnemyName;
    } else if (type == "UpdateEnemyHealth") {
        return MessageType::UpdateEnemyHealth;
    } else if (type == "EnemyDied") {
        return MessageType::EnemyDied;
    } else if (type == "NewBullet") {
        return MessageType::NewBullet;
    } else if (type == "RemoveBullet") {
        return MessageType::RemoveBullet;
    } else if (type == "EnemyRespawned") {
        return MessageType::EnemyRespawned;
    } else if (type == "UpdateEnemyPosition") {
        return MessageType::UpdateEnemyPosition;
    } else if (type == "UpdateEnemyPositionAndVelocity") {
        return MessageType::UpdateEnemyPositionAndVelocity;
    } else if (type == "UpdateEnemyPosVelRot") {
        return MessageType::UpdateEnemyPosVelRot;
    } else if (type == "RequestGameUpdate") {
        return MessageType::RequestGameUpdate;
    } else if (type == "RequestRespawn") {
        return MessageType::RequestRespawn;
    } else {
        return MessageType::Unknown;
    }
}




#endif //MESSAGETYPES_H
