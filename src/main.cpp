#include <iostream>
#include <string>
#include "command_parser.hpp"
#include "key_value_store.hpp"



int main()
{
    
    KeyValueStore storage;
    std::string command;

    const std::string snapshot_file = "snapshot.txt";
    auto save_snapshot = [&storage, &snapshot_file]() -> bool
    {
        try
        {
            storage.save_to_file(snapshot_file);
            return true;
        }
        catch (const std::exception& error)
        {
            std::cerr
                << "Failed to save snapshot: "
                << error.what() << '\n';

            return false;
        }
    };

    try
    {
        storage.load_from_file(snapshot_file);
    }
    catch (const std::exception& error)
    {
        std::cerr << "Failed to load snapshot: "
                  << error.what() << '\n';
        return 1;
    }


    std::cout << "Mini Redis started\n";


    while (true)
    {
        std::cout << "> ";

        if (!std::getline(std::cin, command))
        {
            break;
        }        
        

        Command parsed = parse_command(command);

        if (parsed.operation.empty())
        {
            continue;
        }
        // std::cout << operation << '\n'<< ewkey << '\n'<< value << '\n';

        if (parsed.operation == "SET")
        {
            if (parsed.key.empty() || parsed.value.empty())
            {
                std::cout << "Error: SET requires key and value\n";
                continue;
            }


            if (parsed.ttl.has_value())
            {
                storage.set(parsed.key, parsed.value, parsed.ttl.value());
            }
            else
            {
                storage.set(parsed.key, parsed.value);
            }

            if (!save_snapshot())
            {
                return 1;
            };
            
        }
        

        // Fix for loop to print exact key value/*
        else if (parsed.operation == "GET")
        {
            if (parsed.key.empty())
            {
                std::cout << "Error: GET requires key\n";
            }
            else if (!parsed.value.empty())
            {
                std::cout << "Error: GET does not accept value\n";
            }

           
            else
            { 
                
                auto it = storage.get(parsed.key);
                if (!it.has_value()) 
                { 
                    std::cout << "Error: Key not found:/\n";
                } 
                else 
                { 
                    std::cout << it.value() << "\n";
                    
                }
            } 
        } 
        
        

        else if (parsed.operation == "DEL")
        {
            if (parsed.key.empty())
            {
                std::cout << "Error: DEL requires key\n";
            }
            else if (!parsed.value.empty())
            {
                std::cout << "Error: DEL does not accept value\n";
            }
            else
            {
            
                auto removed = storage.del(parsed.key);
            
                if (removed)
                {
                    if (!save_snapshot())
                    {
                        return 1;
                    }
                    std::cout << "Key is removed\n";
                }
                else
                {
                    std::cout << "Error: Key not found\n";
                }
            }
        }

        else if (parsed.operation == "EXISTS")
        {
            if (parsed.key.empty())
            {
                std::cout << "Error: EXISTS requires key\n";
            }
            else if (!parsed.value.empty())
            {
                std::cout << "Error: EXISTS does not accept value\n";
            }

            else
            {
                auto it = storage.exists(parsed.key);

                if (it)
                {
                    std::cout << "1\n";
                }
                else
                {
                    std::cout << "0\n";
                }
            } 
        }
        
        else if (parsed.operation == "EXIT")
        {
            if (!parsed.key.empty() || !parsed.value.empty())
            {
                std::cout << "Error: EXIT does not accept arguments\n";
            }
            else
            {
                break;
            }
        }

        else if (parsed.operation == "HELP")
        {
            if (!parsed.key.empty() || !parsed.value.empty())
            {
                std::cout << "Error: HELP does not accept arguments\n";
            }
            else
            {
                std::cout << "Commands:\n"
                        << "  SET key value\n"
                        << "  GET key\n"
                        << "  DEL key\n"
                        << "  EXISTS key\n"
                        << "  HELP\n"
                        << "  EXIT\n";
            }
        }

        else
        {
            std::cout << "Error: unknown command\n";
        }
    }

    return 0;
}