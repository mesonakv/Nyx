#pragma once
class Player;

class PlayerPanel {
public:
    void Initialize(Player& player) { player_ = &player; }
    void Draw();

private:
    Player* player_ = nullptr;
};