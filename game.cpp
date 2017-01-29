#include<SFML/Graphics.hpp>

#include<memory>
#include<vector>
#include<cmath>
#include<ctime>
#include<cstdlib>
#include<array>
#include<sstream>

const int WIDTH = 1024;
const int HEIGHT = 768;


float GetNormal(sf::Vector2f& vec)
{
	return sqrtf((vec.x*vec.x) + (vec.y*vec.y));
}

float GetNormal(float px, float py)
{
	return sqrtf(px*px+py*py);
}


sf::Texture GAME_TEXTURE;
sf::Font GAME_FONT;


struct ParticleSystem
{

	struct Particle
	{
		sf::Vector2f vel;
		sf::Time time;
	};

	sf::VertexArray vertices;
	std::vector<Particle> particles;
	sf::Time time;
	sf::Vector2f position;
	float angle;

	void resetParticle(size_t index)
	{
		float ang = angle - 30 + (rand() % 60);
		float speed = (std::rand() % 40) + 80.f;

		ang *= 3.1417f / 180.f;

		particles[index].vel = sf::Vector2f(std::cos(ang) * speed, std::sin(ang) * speed);
		particles[index].time = sf::milliseconds((std::rand() % 2000) + 1000);

		vertices[index].position = position;
	}

	void update(sf::Time& dt, bool gennew)
	{
		for (size_t i = 0; i < particles.size(); ++i)
		{
			Particle& p = particles[i];
			p.time -= dt;

			if (p.time <= sf::Time::Zero)
			{
				if (gennew)
					resetParticle(i);
				else
					p.time = sf::milliseconds((std::rand() % 1500) );
			}

			vertices[i].position += p.vel * dt.asSeconds();

			float ratio = p.time.asSeconds() / time.asSeconds();
			vertices[i].color.a = static_cast<sf::Uint8>(ratio * 255);
			vertices[i].color.r = 160;
			vertices[i].color.g = 160;
		}
	}

	void move(float x, float y, float ang)
	{
		position.x = x;
		position.y = y;
		angle = ang;
	}

	void move(sf::Vector2f pos, float ang)
	{
		position = pos;
		angle = ang;
	}

	ParticleSystem(float x, float y, float ang, unsigned int count) :
		particles(count),
		vertices(sf::Points, count),
		time(sf::seconds(3)),
		position(x, y),
		angle(ang)
	{
	}

};


struct Boat
{
	sf::Sprite base;

	int health;

	float max_vel;
	sf::Vector2f vel;
	ParticleSystem parsys;

	void accelerate(sf::Time& time)
	{
		if (GetNormal(vel) < max_vel*time.asSeconds())
		{
			vel.x += 4.5f*time.asSeconds()*cosf(base.getRotation()*0.017453f);
			vel.y += 4.5f*time.asSeconds()*sinf(base.getRotation()*0.017453f);
		}
	}

	void update(sf::Time& time)
	{
		auto prev = base.getPosition();

		vel.x -= vel.x*2.6f*time.asSeconds();
		vel.y -= vel.y*2.6f*time.asSeconds();

		base.move(vel);

		parsys.move(prev, 57.295f * atan2f(prev.y - base.getPosition().y, prev.x - base.getPosition().x));

		parsys.update(time, GetNormal(vel) > max_vel*0.1*time.asSeconds());
	}

	Boat(float x, float y) :
		base(GAME_TEXTURE, { 0,0,16,16 }),
		health(10),
		max_vel(128.f),
		parsys(x,y,0.f,200)
	{
		base.setScale(5.f, 5.f);
		base.setOrigin(8.f, 8.f);
		base.setPosition(x, y);
	}

};

enum ARMOR {
	FLESH, ROCK, BONE
};

struct MonsterStats
{
	int health;
	int damage;

	int armor;

	float move_speed;

	ARMOR armor_type;
};

struct MonsterType
{
	std::string name;

	sf::IntRect rect;
	MonsterStats stats;

	MonsterType(const std::string& name, sf::IntRect rect, MonsterStats stats):
		name(name),
		rect(rect),
		stats(stats)
	{
	}
};

MonsterType fishor("Fishor", { 0,48,8,8 }, {4, 2, 0, 40.f, FLESH });
MonsterType karkun("Karkun", { 8,48,8,8 }, { 8, 4, 0, 55.f, FLESH });
MonsterType tornal("Tornal", { 16,48,8,8 }, { 12, 5, 1, 62.f, FLESH });

struct Monster
{
	MonsterType& kind;
	MonsterStats stats;

	sf::Sprite base;

	bool hit;

	float tim;
	int anim;

	/*Monster& operator= (const Monster & mon) {
		Monster m2(mon.base.getPosition().x, mon.base.getPosition().y, mon.kind);
		m2.stats = mon.stats;
		m2.hit = mon.hit;
		return m2;
	}*/

	void update(sf::Time& time, Boat& boat)
	{
		auto dif = boat.base.getPosition() - base.getPosition();
		float normal = GetNormal(dif);

		tim += time.asSeconds();

		if (tim > 0.25f)
		{
			tim = 0.f;
			anim = (anim == 0) ? 1 : 0;
			base.setTextureRect({ kind.rect.left, kind.rect.top + anim*kind.rect.height, kind.rect.width, kind.rect.height });
		}

		if (normal <= stats.move_speed)
		{
			boat.health -= stats.damage;
			hit = true;
		}
		else
		{
			dif *= (time.asSeconds()*stats.move_speed) / normal;
			base.move(dif);
			base.setRotation(atan2f(dif.y, dif.x)*57.295f);
		}
	}

	Monster(float x, float y, MonsterType& kind):
		kind(kind),
		base(GAME_TEXTURE, kind.rect),
		hit(false),
		tim(0.f),
		anim(0)
	{
		stats = kind.stats;
		base.setPosition(x, y);
		base.setOrigin(static_cast<float>(kind.rect.width/2), static_cast<float>(kind.rect.height/2));
		base.setScale(4.f, 4.f);
	}
};

std::unique_ptr<Monster> CreateMonster(float x, float y, MonsterType& kind)
{
	return std::unique_ptr<Monster>(new Monster(x, y, kind));
}


struct BulletType {
	sf::IntRect rect;

	float max_dis;
	float speed;

	int max_targs;
};


const int BULLET_SIZE = 8;

BulletType PISTOL_BULLET = { {0,32,BULLET_SIZE, BULLET_SIZE}, 600.f, 250.f, 1 };
BulletType METEOR_BULLET = { { 8,32,BULLET_SIZE, BULLET_SIZE }, 700.f, 325.f, 3 };
BulletType LIGHT_BULLET = { { 16,32,BULLET_SIZE, BULLET_SIZE }, 250.f, 400.f, 2 };

enum WeaponType
{
	WTHEALTH, WTPISTOL, WTSMG, WTMETEOR, WTLIGHT, WTGEM
};

struct Weapon
{
	std::string name;

	BulletType& bullet;
	WeaponType kind;

	int damage;
	int bullets;

	float cd;
	float max_cd;

	Weapon(const std::string& name, WeaponType kind, BulletType& bull, int damage, int bullets, float cd) :
		name(name),
		bullet(bull),
		kind(kind),
		damage(damage),
		bullets(bullets),
		cd(cd),
		max_cd(cd)
	{

	}
};


enum CrateClass
{
	CCHELP, CCWEAPON, CCTECH
};

struct CrateType
{
	WeaponType id;
	sf::IntRect rect;

	CrateClass kind;

	int health;
	int bullets;
};


CrateType HEALTH = { WTHEALTH, {16,64,8,8}, CCHELP, 5, 0 };
CrateType PISTOL = { WTPISTOL, {0,64,8,8 }, CCWEAPON, 0, 16 };
CrateType SMG = { WTSMG,{ 8,64,8,8 }, CCWEAPON, 0, 42 };
CrateType METEOR = { WTMETEOR,{ 24,64,8,8 }, CCWEAPON, 0, 4 };
CrateType GEM = { WTGEM, {32,64,8,8}, CCTECH, 0, 0 };
CrateType LIGHTNING = { WTLIGHT, {40,64,8,8}, CCWEAPON, 0, 50 };


struct Crate
{
	sf::Sprite base;

	CrateType& kind;

	bool picked;

	Crate(float x, float y, CrateType& kind):
		base(GAME_TEXTURE, kind.rect),
		kind(kind),
		picked(false)
	{
		base.setScale(4.f, 4.f);
		base.setOrigin(kind.rect.width / 2, kind.rect.height / 2);
		base.setPosition(x, y);
	}

};



struct Bullet
{
	sf::Sprite base;

	Weapon& weapon;

	bool hit;

	float dis;
	int targs;

	std::map<Monster*, bool> hitted;


	void update(sf::Time& time, std::vector<std::unique_ptr<Monster>>& mons)
	{
		for (auto& mon : mons)
		{
			if( hitted.count(mon.get()) <= 0 )
			{
				sf::Vector2f dif = base.getPosition() - mon->base.getPosition();
				float norm = GetNormal(dif);

				if (norm < 20.f)
				{
					mon->stats.health -= weapon.damage;
					targs--;
					hitted[mon.get()] = true;

					if (targs <= 0)
						break;
				}
			}
		}

		hit = (dis > weapon.bullet.max_dis || targs==0);

		if (!hit)
		{
			float ang = base.getRotation() * 0.01745f;
			float spd = time.asSeconds() * weapon.bullet.speed;

			base.move( spd * cosf(ang), spd * sinf(ang));
			dis += spd;
		}
	}

	Bullet(float x, float y, float angle, Weapon& weap):
		base(GAME_TEXTURE, weap.bullet.rect),
		weapon(weap),
		hit(false),
		dis(0.f),
		targs(weap.bullet.max_targs)
		
	{
		base.setRotation(angle);
		base.setPosition(x, y);
		base.setOrigin( static_cast<float>(BULLET_SIZE / 2), static_cast<float>(BULLET_SIZE / 2));
		base.setScale(3.f, 3.f);
	}

};


struct Player
{
	sf::Sprite base;

	std::vector<Weapon> weapons;
	size_t current_weapon;

	bool down;
	sf::Clock animator;

	void animate(Boat& boat)
	{
		int anim = 0;
		int y = down ? 0 : 16;

		float vel = GetNormal(boat.vel);

		if (down && vel > 0.1f)
		{
			int spd = static_cast<int>(vel);
			int fix = 1 + spd % 4;
			spd = 1000 - fix * 250;

			anim = animator.getElapsedTime().asMilliseconds() / spd % 4;
		}

		base.setTextureRect({16*(anim+1),y,16,16});
	}



	void update(sf::Time& time, Boat& boat)
	{
		base.setPosition(boat.base.getPosition());

		animate(boat);

		for (auto& wea : weapons)
		{
			if (wea.cd > 0.f) wea.cd -= time.asSeconds();
		}


		if (down)
		{
			base.setRotation(boat.base.getRotation());
			base.rotate(180);

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) || sf::Keyboard::isKeyPressed(sf::Keyboard::W))
				boat.accelerate(time);
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) || sf::Keyboard::isKeyPressed(sf::Keyboard::A))
				boat.base.rotate(-30.f * time.asSeconds());
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) || sf::Keyboard::isKeyPressed(sf::Keyboard::D))
				boat.base.rotate(30.f * time.asSeconds());
		}
	}

	int findWeapon(WeaponType who)
	{
		for (size_t x = 0; x < weapons.size(); x++)
		{
			if (weapons[x].kind == who)
				return x;
		}

		return -1;
	}

	bool fire()
	{
		if (weapons[current_weapon].cd < 0.f && weapons[current_weapon].bullets>0)
		{
			//sf::Vector2f pos = base.getPosition();
			weapons[current_weapon].bullets--;
			weapons[current_weapon].cd = weapons[current_weapon].max_cd;

			return true;
		}

		return false;
	}

	Player(float x, float y) :
		base(GAME_TEXTURE, { 16,0,16,16 }),
		current_weapon(0),
		down(true)
	{
		base.setPosition(x, y);
		base.setScale(5.f, 5.f);
		base.setOrigin(8.f, 8.f);

		weapons.push_back( Weapon("Pistol", WTPISTOL, PISTOL_BULLET, 2, 8, 0.6f) );
	}
};

struct GameWindow
{
	sf::RenderWindow window;
	sf::Clock clock;
	sf::View view;

	Boat boat;
	Player player;

	std::vector<std::unique_ptr<Monster>> monsters;
	std::vector<std::unique_ptr<Bullet>> bullets;
	std::vector<std::unique_ptr<Crate>> crates;

	sf::VertexArray tileset;

	sf::RenderTexture render_light;
	sf::Sprite real_light;

	sf::Texture tex_light;
	sf::Sprite light;

	sf::Clock monster_clock;
	sf::Clock crate_clock;

	sf::FloatRect total_area;
	sf::FloatRect areas[3][3];
	int collected_gems;

	bool loop;
	bool placed;

	sf::Text won_text;
	sf::Text weapon_text;

	sf::Text help_text;
	sf::RectangleShape help_rect;
	sf::Text help_info;

	bool show_help;
	bool show_intro;

	sf::Texture intro_tex;
	sf::Sprite intro;

	sf::Text health_text;

	bool preload()
	{
		srand(time(NULL));

		intro_tex.loadFromFile("graphics/intro.png");
		intro.setTexture(intro_tex);

		GAME_FONT.loadFromFile("graphics/yoster.ttf");
		GAME_TEXTURE.loadFromFile("graphics/units.png");

		tex_light.loadFromFile("graphics/light.png");
		light.setTexture(tex_light);

		window.setVerticalSyncEnabled(true);

		won_text.setPosition(WIDTH / 2, HEIGHT / 2);
		won_text.setOrigin(won_text.getGlobalBounds().width / 2.f, won_text.getGlobalBounds().height / 2.f);

		weapon_text.setPosition(4.f, 2.f);

		help_text.setPosition(WIDTH*0.75f, 2.f);

		help_rect.setOrigin(help_rect.getSize() / 2.f);
		help_rect.setPosition(WIDTH / 2, HEIGHT / 2);
		help_rect.setFillColor(sf::Color(0,0,0,160));
		help_rect.setOutlineColor(sf::Color::White);
		help_rect.setOutlineThickness(1.f);

		help_info.setPosition(help_rect.getPosition()-help_rect.getSize()/2.f);
		help_info.move(4.f, 2.f);

		health_text.setPosition(WIDTH / 2, 2.f);
		health_text.setOrigin(health_text.getGlobalBounds().width / 2.f, 0.f);

		loop = true;

		return loop;
	}

	void events()
	{
		sf::Event ev;

		while (window.pollEvent(ev))
		{
			if (ev.type == sf::Event::Closed)
				loop = false;
			else if (ev.type == sf::Event::KeyPressed)
			{
				if (ev.key.code == sf::Keyboard::Space)
					player.down = !player.down;
				else if (ev.key.code == sf::Keyboard::H || ev.key.code == sf::Keyboard::Escape)
				{
					if(!show_intro)
						show_help = !show_help;

					show_intro = false;
				}
				else if (ev.key.code == sf::Keyboard::R)
					restart();
				else if (ev.key.code == sf::Keyboard::Num1)
					player.current_weapon = 0;
				else if (ev.key.code == sf::Keyboard::Num2 && player.weapons.size() > 1)
					player.current_weapon = 1;
				else if (ev.key.code == sf::Keyboard::Num3 && player.weapons.size() > 2)
					player.current_weapon = 2;
				else if (ev.key.code == sf::Keyboard::Num4 && player.weapons.size() > 3)
					player.current_weapon = 3;

			}
		}
	}

	void lightApply()
	{
		light.setOrigin(light.getGlobalBounds().width / 2, light.getGlobalBounds().height / 2);
		//light.setPosition(player.base.getPosition());
		light.setPosition( static_cast<float>(render_light.getSize().x/2), static_cast<float>(render_light.getSize().y/2));

		render_light.clear();
		render_light.draw(light, sf::BlendAdd);
		render_light.display();
		real_light.setTexture(render_light.getTexture());

		real_light.setPosition(player.base.getPosition());
		real_light.setOrigin(real_light.getGlobalBounds().width / 2.f, real_light.getGlobalBounds().height / 2.f);
	}

	void fire()
	{
		if (player.fire())
		{
			auto mpos = sf::Mouse::getPosition(window);
			auto fpos = window.mapPixelToCoords(mpos, view);
			auto dif = player.base.getPosition();
			float ang = atan2f(fpos.y - dif.y, fpos.x-dif.x) * 57.295f;

			bullets.push_back(std::unique_ptr<Bullet>(new Bullet(dif.x, dif.y, ang, player.weapons[player.current_weapon])));
		}
	}

	sf::Vector2i getPlace(bool external)
	{
		float partx = total_area.width / 3.f;
		float party = total_area.height / 3.f;

		auto pos = boat.base.getPosition();

		sf::Vector2i where(static_cast<int>(pos.x / partx), static_cast<int>(pos.y / party));

		if (external)
		{
			std::vector<sf::Vector2i> possible;

			for (int x = 0; x < 3; x++)
			{
				for (int y = 0; y < 3; y++)
				{
					if (where.x != x || where.y != y) possible.push_back({ x,y });
				}
			}

			where = possible[rand() % possible.size()];
		}

		return where;
	}

	sf::Vector2f getRandomPlace(bool external)
	{
		auto vec = getPlace(external);
		int partx = total_area.width / 3.f;
		int party = total_area.height / 3.f;

		return sf::Vector2f( rand() % partx + vec.x*partx, rand() % party + vec.y*party);
	}

	void createCrates()
	{
		if (crate_clock.getElapsedTime().asSeconds() > 5.f)
		{
			crate_clock.restart();

			std::array<CrateType*, 5> what{ {&HEALTH, &PISTOL, &SMG, &METEOR, &LIGHTNING} };

			auto pos = getRandomPlace(true);

			if (placed)
			{
				if (crates.size() < 25)
					crates.push_back(std::unique_ptr<Crate>(new Crate(pos.x, pos.y, *what[rand() % what.size()])));
			}
			else
			{
				placed = true;
				crates.push_back(std::unique_ptr<Crate>(new Crate(pos.x, pos.y, GEM)));
			}

			//std::cout << pos.x - boat.base.getPosition().x << " " << pos.y-boat.base.getPosition().y << std::endl;
		}
	}

	void grabCrates(sf::Time& time)
	{
		for (auto& cra : crates)
		{
			if (cra->base.getGlobalBounds().intersects(player.base.getGlobalBounds()))
			{
				cra->picked = true;

				if (cra->kind.kind == CCWEAPON)
				{
					int weap = player.findWeapon(cra->kind.id);
					if (weap != -1)
						player.weapons[weap].bullets += cra->kind.bullets;
					else
					{
						switch (cra->kind.id)
						{
							case WTPISTOL:
								player.weapons.push_back(Weapon("Pistol", WTPISTOL, PISTOL_BULLET, 3, 16, 0.6f));
								break;
							case WTSMG:
								player.weapons.push_back(Weapon("SMG", WTSMG, PISTOL_BULLET, 2, 42, 0.2f));
								break;
							case WTMETEOR:
								player.weapons.push_back(Weapon("Meteorites", WTMETEOR, METEOR_BULLET, 8, 4, 1.f));
								break;
							case WTLIGHT:
								player.weapons.push_back(Weapon("Lightning", WTLIGHT, LIGHT_BULLET, 1, 50, 0.1f));
						}
					}
				}
				else if (cra->kind.kind == CCHELP)
				{
					boat.health = std::min(10, boat.health+cra->kind.health);
				}
				else if (cra->kind.kind == CCTECH)
				{
					collected_gems++;
					placed = false;
				}
			}
		}
	}

	void update()
	{
		auto mpos = sf::Mouse::getPosition(window);
		auto fpos = window.mapPixelToCoords(mpos, view);
		auto time = clock.restart();

		if (show_intro || show_help)
			return;

		boat.update(time);
		player.update(time, boat);

		sf::FloatRect inter;
		auto res = boat.base.getGlobalBounds().intersects(total_area, inter);

		if (inter.width <= 0.9*boat.base.getGlobalBounds().width || inter.height <= 0.9*boat.base.getGlobalBounds().height)
		{
			sf::Vector2f middle(total_area.width / 2.f, total_area.height / 2.f);
			auto dif = middle-boat.base.getPosition();
			float norm = time.asSeconds()/GetNormal(dif);

			boat.base.move(128.f*norm*dif.x, 128.f*norm*dif.y);
		}

		createCrates();
		grabCrates(time);

		if (!player.down)
		{
			auto dif = fpos - player.base.getPosition();
			player.base.setRotation(57.29f*atan2(dif.y, dif.x));
		}

		if (sf::Mouse::isButtonPressed(sf::Mouse::Left) && !player.down)
			fire();

		view.setCenter(player.base.getPosition());

		for (auto& bull : bullets)
		{
			bull->update(time, monsters);
		}

		if ( monster_clock.getElapsedTime().asSeconds() > (3.f - collected_gems*0.15f) )
		{
			if (monsters.size() < 25 + collected_gems*5)
			{
				//std::array<MonsterType*, 2> mt{ {&fishor, &karkun} };
				std::array<MonsterType*, 5> mt{ { &fishor, &fishor, &karkun, &karkun, &tornal } };
				
				auto pos = getRandomPlace(false);
				MonsterType* cmon = mt[collected_gems];

				auto wha = CreateMonster(pos.x, pos.y, *cmon);

				if (!wha->base.getGlobalBounds().intersects(boat.base.getGlobalBounds()))
				{
					monsters.push_back(CreateMonster(pos.x, pos.y, *cmon));
					monster_clock.restart();
				}
			}
			else
				monster_clock.restart();
		}

		bullets.erase(
			std::remove_if(bullets.begin(), bullets.end(),
				[](const std::unique_ptr<Bullet>& mon) { return (mon->hit); }),
			bullets.end());

		for (auto& mon : monsters)
		{
			mon->update(time, boat);
		}

		monsters.erase(
			std::remove_if(monsters.begin(), monsters.end(),
				[](const std::unique_ptr<Monster>& mon) { return (mon->hit || mon->stats.health <= 0); }),
			monsters.end());

		crates.erase(
			std::remove_if(crates.begin(), crates.end(),
				[](const std::unique_ptr<Crate>& mon) { return (mon->picked); }),
			crates.end());


		std::stringstream stri;
		stri << player.weapons[player.current_weapon].bullets;

		weapon_text.setString("Weapon: " + player.weapons[player.current_weapon].name + " " + stri.str() );

		stri.str(std::string());
		stri << boat.health;

		health_text.setString("Health: " + stri.str());

		if (collected_gems >= 5)
		{
			monsters.clear();
			//bullets.clear();

			won_text.setString("After destroying the last gem,\nthe creatures suddenly dissapear.\nIt's time to go back home.\nVICTORY");
			won_text.setOrigin(won_text.getGlobalBounds().width / 2.f, won_text.getGlobalBounds().height / 2.f);
		}

	}

	void draw()
	{
		window.clear();

		if (show_intro)
		{
			window.draw(intro);
			window.display();
			return;
		}

		window.setView(view);
		window.draw(tileset, &GAME_TEXTURE);

		window.draw(boat.parsys.vertices);

		for (auto& mon : crates)
		{
			window.draw(mon->base);
		}

		for (auto& mon : monsters)
		{
			window.draw(mon->base);
		}

		window.draw(boat.base);
		window.draw(player.base);

		for (auto& mon : bullets)
		{
			window.draw(mon->base);
		}

		//window.setView(window.getDefaultView());
		lightApply();
		//window.setView(view);
		window.draw(real_light, sf::BlendMultiply);
		window.setView(window.getDefaultView());

		window.draw(weapon_text);
		window.draw(health_text);
		window.draw(help_text);

		if (show_help)
		{
			window.draw(help_rect);
			window.draw(help_info);
		}
		else
			window.draw(won_text);

		window.display();
	}

	void generate()
	{
		unsigned int wid = static_cast<unsigned int>(total_area.width / 32.f);
		unsigned int hei = static_cast<unsigned int>(total_area.height / 32.f);

		float tam = 32;
		float gtam = 8;

		float partx = total_area.width / 3.f;
		float party = total_area.height / 3.f;

		for (unsigned int x = 0; x < 3; x++)
		{
			for (unsigned int y = 0; y < 3; y++)
			{
				areas[x][y].left = static_cast<float>(x)*partx;
				areas[x][y].top = static_cast<float>(x)*partx;
				areas[x][y].width = partx;
				areas[x][y].height = party;
			}
		}
		

		for (unsigned int i = 0; i < wid; i++)
		{
			for (unsigned int j = 0; j < hei; j++)
			{
				sf::Vertex* quad = &tileset[(i + j * wid) * 4];

				quad[0].position = sf::Vector2f(i * tam, j * tam);
				quad[1].position = sf::Vector2f(i * tam + tam, j * tam);
				quad[2].position = sf::Vector2f(i * tam + tam, j * tam + tam);
				quad[3].position = sf::Vector2f(i * tam, j * tam + tam);

				quad[0].texCoords = sf::Vector2f(0.f, 2*gtam);
				quad[1].texCoords = sf::Vector2f(gtam, 2*gtam);
				quad[2].texCoords = sf::Vector2f(gtam, 3*gtam);
				quad[3].texCoords = sf::Vector2f(0.f, 3*gtam);
			}
		}
	}

	void restart()
	{
		monster_clock.restart();
		clock.restart();

		collected_gems = 0;
		placed = false;

		view = sf::View({ 0.f,0.f,static_cast<float>(WIDTH),static_cast<float>(HEIGHT) });
		boat = Boat(static_cast<float>(WIDTH / 2), static_cast<float>(HEIGHT / 2));
		player = Player(boat.base.getPosition().x, boat.base.getPosition().y);

		monsters.clear();
		bullets.clear();
		crates.clear();

		won_text.setString("");
		weapon_text.setString("Weapon: Pistol");
		show_help = false;
	}

	GameWindow() :
		window(sf::VideoMode(WIDTH, HEIGHT), "Last Sea"),
		view({ 0.f,0.f,static_cast<float>(WIDTH),static_cast<float>(HEIGHT) }),
		boat(static_cast<float>(WIDTH / 2), static_cast<float>(HEIGHT / 2)),
		player(boat.base.getPosition().x, boat.base.getPosition().y),
		total_area({ 0.f,0.f,4000.f, 4000.f }),
		collected_gems(0),
		loop(false),
		placed(false),
		won_text("", GAME_FONT),
		weapon_text("Weapon: Pistol", GAME_FONT, 24),
		help_text("Press H for help", GAME_FONT, 24),
		help_rect({WIDTH/2,HEIGHT/2 + 20.f}),
		help_info("Controls:\nUP-W : Move forward\nA-LEFT : Rotate left\nD-RIGHT : Rotate right\nSpace : Leave-take oars\nLeft Click : Shoot when you're up\n1-2-3-4 : Swap weapons\nR : Restart\nLook for ammo-health crates\n\nFIND 5 GEMS TO WIN", GAME_FONT, 24),
		show_help(true),
		show_intro(true),
		health_text( "Health: 10", GAME_FONT, 24 )
	{
		if (preload())
		{
			tileset.setPrimitiveType(sf::Quads);
			tileset.resize( static_cast<int>((total_area.width/32.f) * (total_area.height/32.f)) * 4 );
			render_light.create(WIDTH, HEIGHT);

			generate();

			while (loop)
			{
				if (boat.health <= 0) restart();

				events();
				update();
				draw();
			}
		}
	}

};



int main()
{
	GameWindow abc;
}
