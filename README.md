News 2015-08-25: After 17 years of beta testing NiKom v1.61 is released!

Binary distributions can be found at http://www.nikom.org/

NiKom is a bulletin board system (BBS) for AmigaOS that was originally developed by me (Niklas Lindholm) between 1990 and 1996. Then Tomas Kärki took over development for a few years until activity on the project died. It's entirely in Swedish (for now) and follows the "KOM" model of command driven bulletin board system that was fairly popular in Sweden in the 90's. (There is currently a menu driven alternative as well but I suspect there is not much interest for it and that it will get killed off.)

This repo on GitHub is an attempt to bring the source code back to life. It's a result of discussions with Fabian Norlin who has been running one of the longest running NiKom BBSes, Fabbes BBS. In 2014 he managed to get his system up and running again and we talked about the possibility of fixing some issues with the code. Ha also had gotten his hands on the last version of the source code as Tomas left it off. I have taken this source code archive and pulled out the important parts, reorganized it a bit and made sure it compiles.

I don't know what the activity on this will be going forward and if there are other people out there who are eager to contribute but for the changes that are made I have the following guidelines in mind.

1. New code should be reasonably well designed and readable. Just because there is a lot of crappy existing code there's no excuse to add more crappy code.
2. When changing existing code make an effort to clean it up. Be a good boy scout and leave the code a bit better than you found it.
3. The "swenglishness" of the code should be reduced over time. The code should as far as possible be in English and not in Swedish.

I have used a clone of the hard drive contents of my old Amiga 3000 now running in FS-UAE to compile NiKom. It's running AmigaOS 3.1 and the C compiler is SAS/C 6.56. There may be assumptions in the build files that only applies to my environment. This will need to be fixed as discovered and needed.

Also see the wiki pages: https://github.com/punktniklas/NiKom/wiki

NiKom is published as open source under the MIT License. See LICENSE.txt.
