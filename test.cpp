#include <iostream>
#include <string>
#include "test.h"

using namespace std;

int main()
{
    WearableItem armor("철 갑옷", 1500, 1, "armor.png");

    // WeaponItem a = WeaponItem ("엑스칼리버", 500, 2, 50, 1.5f, "sword.png");
    // a.attack();
    WeaponItem weapon_1 ("sword", 5, 1, 5, 10, "./img/sword.jpg", false);
    WeaponItem weapon_2 ("bow", 3, 2, 0.5f, 10, "./img/bow.jpg", false);
    cout << weapon_1.get_name() << " 생성 완료!" << endl;
    cout << "데미지: " << weapon_1.attack_damage << endl;
    return 0;
}