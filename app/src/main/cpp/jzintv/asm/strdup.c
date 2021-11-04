char * strdup(const char *s)
{
    int len = strlen(s) + 1;
    char *new_str = (char *)malloc(len);

    if (new_str) strcpy(new_str, s);

    return new_str;
}
