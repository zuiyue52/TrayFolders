// SPDX-License-Identifier: GPL-3.0-only
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Windows.Forms;

internal sealed class FolderListBox : ListBox
{
    internal void RefreshNames()
    {
        RefreshItems();
    }
}
