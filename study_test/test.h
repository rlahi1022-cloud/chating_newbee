#include <iostream>
#include <string>

using namespace std;

class ExampleClass
{
    public:
    int num;
    static int count;

    ExampleClass (int num)
    {
        count++;
        this -> num = num;
    }
    void show_instance_num()
    {
        cout<< this -> count << endl;
    }
    void show_num()
    {
        cout<< this -> num << endl;
    }
    void set_num(int num)
    {
        this -> num = num;
    }
    int get_num()
    {
        return num;
    }

    ~ExampleClass()
    {
        count--;
    }
};

class ChildExampleClass : public ExampleClass
{
    public:
    ChildExampleClass(int num) : ExampleClass(num){}
    void display()
    {
        cout << "Child class num : " << get_num() << endl;
    }
    using ExampleClass::ExampleClass;
};

class AnotherChildExampleClass : public ExampleClass
{
    public:
    AnotherChildExampleClass(int num) : ExampleClass(num){}
    void ChildDisplay()
    {
        cout << "Another Child class num : " << get_num() << endl;
    }
};

int ExampleClass::count = 0;

class Item
{
    public:
    string name;
    int weight;
    string img_path;

    Item (string name, int weight, string img_path="")
    {
        this -> name = name;
        if(weight >0)
        {
            this ->weight =weight;
        }
        else
        {
            this -> weight =0;
        }
        this ->img_path =img_path;
    }
    string get_name()
    {
        return this ->name;
    }
    int get_weight()
    {
        return this -> weight;
    }
    void show_image()
    {
        cout << this -> img_path << endl;
    }
};

class WearableItem : public Item
{
    public :
    short location;
    bool state;

    WearableItem(string name, int weight, short location, string img_paht="", bool state = false) : Item(name, weight, img_path)
    {
        this -> location =location;
        this -> state =state;
    }
    string get_name()
    {
        return "장비 : " + this-> name;
    }
    short get_location()
    {
        return this -> location;
    }
    bool get_state()
    {
        return this -> state;
    }
};

class WeaponItem : public WearableItem 
{
    public :
    int attack_damage;
    float attack_speed;

    WeaponItem(string name, int weight, short location, int atk_damage = 1, float atk_speed =1, string img_path = "", bool state = false) : WearableItem(name, weight, location, img_path, state)
    {
        this -> attack_damage = atk_damage;
        this -> attack_speed = atk_speed;
    };

    int attack ()
    {
        float attack;
        attack = {this -> attack_damage * this -> attack_speed};
        return (int)attack;
    }
};

class MeleeWeapon : public WeaponItem {};
class RangeWeapon : public WeaponItem {};

// int main() 
// {
    // ExampleClass a = ExampleClass(10);
    // ExampleClass b = ExampleClass(20);
    // {
    //     ExampleClass c = ExampleClass(10);
    //     a.show_num();
    //     cout << "c 입니다." << endl;
    // }
    // AnotherChildExampleClass d = AnotherChildExampleClass(30);
    // a.show_num();
    // a.show_instance_num(); 
    // b.show_num();
    // b.show_instance_num();
    // // c.show_num();
    // a.show_instance_num();
    // // c.display();
    // d.ChildDisplay();
    // a.show_num();

//     return 0;
// }