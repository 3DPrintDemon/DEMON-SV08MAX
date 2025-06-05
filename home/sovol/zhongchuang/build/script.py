import configparser
import sys

def update_ini_value(filename, section, key, value):
    config = configparser.ConfigParser()
    config.read(filename)

    if not config.has_section(section):
        config.add_section(section)

    config.set(section, key, value)

    with open(filename, 'w') as configfile:
        config.write(configfile)

if __name__ == '__main__':
    if len(sys.argv) < 5:
        print('Usage: python script.py <filename> <section> <key> <value>')
        sys.exit(1)

    filename = sys.argv[1]
    section = sys.argv[2]
    key = sys.argv[3]
    value = sys.argv[4]

    update_ini_value(filename, section, key, value)
