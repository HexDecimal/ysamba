#pragma once

namespace dl
{
enum class SocialClass
{
  None,
  Slave,
  Plebeian,
  Ruler,
};

enum class Metier
{
  None,
  Farmer,
  Warrior,
  Priest,
  Bricklayer,
  Dressmaker,
  Cook,
  Cacique,
};

struct SocietyAgent
{
  enum class State
  {
    Idle,
    Walking,
    Harvesting,
  };

  std::string id;
  std::string society_id;
  std::string name;
  SocialClass social_class;
  Metier metiers;
  State state = State::Harvesting;
  double time_to_next_action = 0.0;
};
}  // namespace dl
